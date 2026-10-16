#include <Wire.h>

// Triggers the BMP180 corner cases the datasheet leaves open and prints what
// the sensor does, so that the output of the VP can be compared to real
// hardware. Sections Q1 to Q10 follow the open questions in
// src/models/bmp180.h; Q0 records reference values used by the others.
//
// Each case starts from a soft reset. Data register contents are classified
// by comparing them to the reference temperature and pressure conversions.

#define BMP180_ADDR 0x77
#define CHIP_ID 0x55

#define REG_CALIB 0xaa
#define REG_ID 0xd0
#define REG_SOFT_RESET 0xe0
#define REG_CTRL_MEAS 0xf4
#define REG_OUT_MSB 0xf6

#define CMD_TEMPERATURE 0x2e
#define CMD_PRESSURE 0x34
#define CTRL_MEAS_SCO 0x20
#define CTRL_MEAS_MC 0x1f
#define CTRL_MEAS_UNUSED 0x04
#define MC_TEMPERATURE 0x0e
#define MC_PRESSURE 0x14
#define SOFT_RESET_CMD 0xb6
#define OSS(n) ((uint8_t)((n) << 6))

// Q5 and Q7 write to read-only registers and to the reserved registers 0xf3
// and 0xf5. Each is followed by a soft reset, but set this to 0 to skip them.
#define WRITE_PROTECTED_REGISTERS 1

#define TIMEOUT_US 100000UL
#define SETTLE_MS 30
#define NOISE_SAMPLES 16
#define MAX_SNAPSHOTS 40
#define SNAPSHOT_US 1000

int16_t ac1, ac2, ac3, b1, b2, mb, mc, md;
uint16_t ac4, ac5, ac6;

bool refs_valid = false;
uint16_t ref_temperature;
uint16_t ref_pressure;

// I2C access

uint8_t write_regs(uint8_t reg, const uint8_t *data, uint8_t len) {
  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(reg);
  Wire.write(data, len);
  return Wire.endTransmission();
}

uint8_t write_reg(uint8_t reg, uint8_t value) {
  return write_regs(reg, &value, 1);
}

bool read_regs(uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  if (Wire.requestFrom((uint8_t)BMP180_ADDR, len) != len) {
    return false;
  }

  for (uint8_t i = 0; i < len; i++) {
    buf[i] = Wire.read();
  }

  return true;
}

// Returns -1 if the read failed.
int16_t read_reg(uint8_t reg) {
  uint8_t value;
  return read_regs(reg, &value, 1) ? value : -1;
}

// Sensor control

// Writes ctrl_meas and returns the time of the write.
unsigned long start(uint8_t cmd) {
  write_reg(REG_CTRL_MEAS, cmd);
  return micros();
}

// Polls ctrl_meas until sco is cleared. Returns the time since `since` in us,
// or -1 on timeout.
long wait_sco(unsigned long since) {
  while (true) {
    int16_t ctrl = read_reg(REG_CTRL_MEAS);
    unsigned long now = micros();
    if (ctrl >= 0 && !(ctrl & CTRL_MEAS_SCO)) {
      return now - since;
    }
    if (now - since > TIMEOUT_US) {
      return -1;
    }
  }
}

// Returns the time until the chip id can be read again in us, or -1.
long soft_reset() {
  write_reg(REG_SOFT_RESET, SOFT_RESET_CMD);
  unsigned long since = micros();
  while (true) {
    bool ready = read_reg(REG_ID) == CHIP_ID;
    unsigned long now = micros();
    if (ready) {
      return now - since;
    }
    if (now - since > TIMEOUT_US) {
      return -1;
    }
  }
}

void reset_sensor() {
  soft_reset();
  delay(10);
}

bool read_calib() {
  uint8_t buf[22];
  if (!read_regs(REG_CALIB, buf, sizeof(buf))) {
    return false;
  }

  uint16_t words[11];
  for (uint8_t i = 0; i < 11; i++) {
    words[i] = ((uint16_t)buf[2 * i] << 8) | buf[2 * i + 1];
  }

  ac1 = words[0];
  ac2 = words[1];
  ac3 = words[2];
  ac4 = words[3];
  ac5 = words[4];
  ac6 = words[5];
  b1 = words[6];
  b2 = words[7];
  mb = words[8];
  mc = words[9];
  md = words[10];
  return true;
}

// Compensation, see datasheet Figure 4

long compute_b5(long ut) {
  long x1 = ((ut - (long)ac6) * (long)ac5) >> 15;
  long x2 = ((long)mc << 11) / (x1 + md);
  return x1 + x2;
}

long compute_pressure(long up, long b5, uint8_t oss) {
  long b6 = b5 - 4000;
  long x1 = (b2 * ((b6 * b6) >> 12)) >> 11;
  long x2 = (ac2 * b6) >> 11;
  long x3 = x1 + x2;
  long b3 = ((((long)ac1 * 4 + x3) << oss) + 2) >> 2;

  x1 = (ac3 * b6) >> 13;
  x2 = (b1 * ((b6 * b6) >> 12)) >> 16;
  x3 = ((x1 + x2) + 2) >> 2;
  unsigned long b4 = ((unsigned long)ac4 * (unsigned long)(x3 + 32768)) >> 15;
  unsigned long b7 = ((unsigned long)up - b3) * (50000 >> oss);

  long p;
  if (b7 < 0x80000000) {
    p = (b7 << 1) / b4;
  } else {
    p = (b7 / b4) << 1;
  }

  x1 = (p >> 8) * (p >> 8);
  x1 = (x1 * 3038) >> 16;
  x2 = (-7357 * p) >> 16;
  return p + ((x1 + x2 + 3791) >> 4);
}

long raw_ut(const uint8_t *out) {
  return ((long)out[0] << 8) | out[1];
}

long raw_up(const uint8_t *out, uint8_t oss) {
  return (((long)out[0] << 16) | ((long)out[1] << 8) | out[2]) >> (8 - oss);
}

// Output

void print_hex(uint8_t value) {
  if (value < 0x10) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

void print_reg(int16_t value) {
  if (value < 0) {
    Serial.print(F("--"));
  } else {
    print_hex(value);
  }
}

void print_bytes(const uint8_t *buf, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    if (i > 0) {
      Serial.print(' ');
    }
    print_hex(buf[i]);
  }
}

void print_ms(long us) {
  if (us < 0) {
    Serial.print(F("timeout"));
    return;
  }

  Serial.print(us / 1000);
  Serial.print('.');
  Serial.print((us % 1000) / 100);
  Serial.print(F(" ms"));
}

void print_status(uint8_t status) {
  switch (status) {
  case 0:
    Serial.print(F("ack"));
    break;
  case 2:
    Serial.print(F("nack on address"));
    break;
  case 3:
    Serial.print(F("nack on data"));
    break;
  default:
    Serial.print(F("error "));
    Serial.print(status);
  }
}

void section(const __FlashStringHelper *title) {
  Serial.println();
  Serial.println(title);
}

void subsection(const __FlashStringHelper *title) {
  Serial.print(F("  "));
  Serial.println(title);
}

void label(const __FlashStringHelper *text) {
  Serial.print(F("    "));
  Serial.print(text);
  Serial.print(F(": "));
}

uint16_t distance(uint16_t a, uint16_t b) {
  return a > b ? a - b : b - a;
}

const __FlashStringHelper *classify(const uint8_t *out) {
  uint16_t value = ((uint16_t)out[0] << 8) | out[1];
  if (value == 0x8000 && out[2] == 0x00) {
    return F("reset value");
  }
  uint16_t dt = distance(value, ref_temperature);
  uint16_t dp = distance(value, ref_pressure);
  if (dt > 2000 && dp > 2000) {
    return F("unknown");
  }
  return dt <= dp ? F("temperature") : F("pressure");
}

void print_out_buf(const __FlashStringHelper *text, const uint8_t *out, bool ok) {
  label(text);
  if (!ok) {
    Serial.println(F("read failed"));
    return;
  }

  print_bytes(out, 3);
  if (refs_valid) {
    Serial.print(F(" ("));
    Serial.print(classify(out));
    Serial.print(')');
  }
  Serial.println();
}

void print_out(const __FlashStringHelper *text) {
  uint8_t out[3];
  bool ok = read_regs(REG_OUT_MSB, out, sizeof(out));
  print_out_buf(text, out, ok);
}

// Starts a conversion, waits for it and prints time and result. Returns
// whether the data registers could be read into out.
bool run_conversion(const __FlashStringHelper *text, uint8_t cmd, uint8_t *out) {
  unsigned long since = start(cmd);
  long t = wait_sco(since);
  bool ok = read_regs(REG_OUT_MSB, out, 3);

  subsection(text);
  label(F("sco cleared after"));
  print_ms(t);
  Serial.println();
  print_out_buf(F("out"), out, ok);
  return ok && t >= 0;
}

// Writes `first` to ctrl_meas, then `value` to `reg` after delay_ms, and
// reports when sco is cleared and what the data registers contain then and
// SETTLE_MS later.
void overlap(const __FlashStringHelper *text, uint8_t first, uint8_t delay_ms,
             uint8_t reg, uint8_t value) {
  reset_sensor();

  unsigned long t1 = start(first);
  delay(delay_ms);
  write_reg(reg, value);
  unsigned long t2 = micros();
  int16_t ctrl = read_reg(REG_CTRL_MEAS);
  long t = wait_sco(t1);

  uint8_t out1[3], out2[3];
  bool ok1 = read_regs(REG_OUT_MSB, out1, 3);
  delay(SETTLE_MS);
  bool ok2 = read_regs(REG_OUT_MSB, out2, 3);
  int16_t ctrl2 = read_reg(REG_CTRL_MEAS);

  subsection(text);
  label(F("ctrl_meas after 2nd write"));
  print_reg(ctrl);
  Serial.println();
  label(F("sco cleared after"));
  print_ms(t);
  if (t >= 0) {
    Serial.print(F(" (1st write), "));
    print_ms(t - (long)(t2 - t1));
    Serial.print(F(" (2nd write)"));
  }
  Serial.println();
  print_out_buf(F("out when sco cleared"), out1, ok1);
  print_out_buf(F("out 30 ms later"), out2, ok2);
  label(F("ctrl_meas 30 ms later"));
  print_reg(ctrl2);
  Serial.println();
}

// Writes cmd to ctrl_meas and reports what happens.
void single(uint8_t cmd) {
  reset_sensor();

  unsigned long since = start(cmd);
  int16_t ctrl = read_reg(REG_CTRL_MEAS);
  long t = wait_sco(since);

  uint8_t out1[3], out2[3];
  bool ok1 = read_regs(REG_OUT_MSB, out1, 3);
  delay(SETTLE_MS);
  bool ok2 = read_regs(REG_OUT_MSB, out2, 3);
  int16_t ctrl2 = read_reg(REG_CTRL_MEAS);

  Serial.print(F("  ctrl_meas <- "));
  print_hex(cmd);
  Serial.println();
  label(F("ctrl_meas readback"));
  print_reg(ctrl);
  Serial.println();
  label(F("sco cleared after"));
  print_ms(t);
  Serial.println();
  print_out_buf(F("out when sco cleared"), out1, ok1);
  print_out_buf(F("out 30 ms later"), out2, ok2);
  label(F("ctrl_meas 30 ms later"));
  print_reg(ctrl2);
  Serial.println();
}

void probe_write(uint8_t reg, uint8_t value) {
  reset_sensor();
  int16_t before = read_reg(reg);
  uint8_t status = write_reg(reg, value);
  int16_t after = read_reg(reg);
  reset_sensor();
  int16_t after_reset = read_reg(reg);

  Serial.print(F("  "));
  print_hex(reg);
  Serial.print(F(" <- "));
  print_hex(value);
  Serial.print(F(": "));
  print_status(status);
  Serial.print(F(", before "));
  print_reg(before);
  Serial.print(F(", after "));
  print_reg(after);
  Serial.print(F(", after soft reset "));
  print_reg(after_reset);
  Serial.println();
}

struct snapshot {
  unsigned long t;
  uint8_t regs[5]; // ctrl_meas, reserved 0xf5, out_msb, out_lsb, out_xlsb
};

// Starts a conversion and samples ctrl_meas and the data registers every
// SNAPSHOT_US until shortly after sco is cleared. Prints every change.
void trace(const __FlashStringHelper *text, uint8_t cmd) {
  snapshot snaps[MAX_SNAPSHOTS];
  uint8_t n = 0;
  uint8_t after_done = 0;

  unsigned long since = start(cmd);
  while (n < MAX_SNAPSHOTS && after_done < 3) {
    while (micros() - since < (unsigned long)n * SNAPSHOT_US) {
    }

    snapshot &s = snaps[n++];
    if (!read_regs(REG_CTRL_MEAS, s.regs, sizeof(s.regs))) {
      memset(s.regs, 0xff, sizeof(s.regs));
    }
    s.t = micros() - since;
    if (!(s.regs[0] & CTRL_MEAS_SCO)) {
      after_done++;
    }
  }

  subsection(text);
  for (uint8_t i = 0; i < n; i++) {
    bool changed = i == 0 || i == n - 1 ||
                   memcmp(snaps[i].regs, snaps[i - 1].regs, 5) != 0;
    if (!changed) {
      continue;
    }

    Serial.print(F("    "));
    print_ms(snaps[i].t);
    Serial.print(F(": ctrl_meas "));
    print_hex(snaps[i].regs[0]);
    Serial.print(F(", out "));
    print_bytes(snaps[i].regs + 2, 3);
    Serial.print(F(" ("));
    Serial.print(classify(snaps[i].regs + 2));
    Serial.println(')');
  }

  if (after_done == 0) {
    Serial.println(F("    snapshot limit reached before sco was cleared"));
  }
}

struct stats {
  uint8_t n = 0;
  float mean = 0.0;
  float m2 = 0.0;

  void add(float x) {
    n++;
    float delta = x - mean;
    mean += delta / n;
    m2 += delta * (x - mean);
  }

  float rms() const { return n > 1 ? sqrt(m2 / (n - 1)) : 0.0; }
};

// Test cases

void q0_reference() {
  section(F("Q0 reference"));
  label(F("ready after soft reset"));
  print_ms(soft_reset());
  Serial.println();
  delay(10);

  label(F("chip id"));
  print_reg(read_reg(REG_ID));
  Serial.println();
  label(F("ctrl_meas after reset"));
  print_reg(read_reg(REG_CTRL_MEAS));
  Serial.println();
  print_out(F("out after reset"));

  uint8_t calib[22];
  label(F("calibration"));
  if (read_regs(REG_CALIB, calib, sizeof(calib))) {
    print_bytes(calib, sizeof(calib));
    Serial.println();
  } else {
    Serial.println(F("read failed"));
  }

  uint8_t out_t[3], out_p[3];
  bool ok = run_conversion(F("temperature (2e)"), CMD_TEMPERATURE, out_t);
  ok &= run_conversion(F("pressure oss0 (34)"), CMD_PRESSURE | OSS(0), out_p);
  ok &= run_conversion(F("pressure oss1 (74)"), CMD_PRESSURE | OSS(1), out_p);
  ok &= run_conversion(F("pressure oss2 (b4)"), CMD_PRESSURE | OSS(2), out_p);
  ok &= run_conversion(F("pressure oss3 (f4)"), CMD_PRESSURE | OSS(3), out_p);
  if (!ok) {
    Serial.println(F("  reference conversions failed"));
    return;
  }

  ref_temperature = ((uint16_t)out_t[0] << 8) | out_t[1];
  ref_pressure = ((uint16_t)out_p[0] << 8) | out_p[1];
  refs_valid = true;

  long b5 = compute_b5(raw_ut(out_t));
  label(F("temperature"));
  Serial.print(((b5 + 8) >> 4) / 10.0, 1);
  Serial.println(F(" C"));
  label(F("pressure"));
  Serial.print(compute_pressure(raw_up(out_p, 3), b5, 3) / 100.0, 2);
  Serial.println(F(" hPa"));
}

void q1_restart() {
  section(F("Q1 conversion started while another one is ongoing"));
  overlap(F("pressure oss3, temperature after 2 ms"), CMD_PRESSURE | OSS(3), 2,
          REG_CTRL_MEAS, CMD_TEMPERATURE);
  overlap(F("temperature, pressure oss3 after 1 ms"), CMD_TEMPERATURE, 1,
          REG_CTRL_MEAS, CMD_PRESSURE | OSS(3));
  overlap(F("pressure oss3, pressure oss3 again after 10 ms"),
          CMD_PRESSURE | OSS(3), 10, REG_CTRL_MEAS, CMD_PRESSURE | OSS(3));
}

void q2_unknown_control() {
  section(F("Q2 measurement control other than temperature/pressure"));
  // 0xee is temperature with oss bits, 0x0e and 0x14 lack the sco bit.
  const uint8_t cmds[] = {0x20, 0x2f, 0x35, 0x3f, 0xee, 0x0e, 0x14};
  for (uint8_t cmd : cmds) {
    single(cmd);
  }
}

void q3_oss_change() {
  section(F("Q3 oversampling changed during conversion"));
  overlap(F("pressure oss3, oss0 without sco after 2 ms"),
          CMD_PRESSURE | OSS(3), 2, REG_CTRL_MEAS, MC_PRESSURE | OSS(0));
  overlap(F("pressure oss0, oss3 without sco after 1 ms"),
          CMD_PRESSURE | OSS(0), 1, REG_CTRL_MEAS, MC_PRESSURE | OSS(3));
}

void q4_reset_during_conversion() {
  section(F("Q4 soft reset during conversion"));
  overlap(F("pressure oss3, soft reset after 2 ms"), CMD_PRESSURE | OSS(3), 2,
          REG_SOFT_RESET, SOFT_RESET_CMD);
  overlap(F("temperature, soft reset after 1 ms"), CMD_TEMPERATURE, 1,
          REG_SOFT_RESET, SOFT_RESET_CMD);
}

void q5_protected_writes() {
  section(F("Q5 writes to read-only and reserved registers"));
#if WRITE_PROTECTED_REGISTERS
  probe_write(REG_ID, 0x00);
  probe_write(REG_CALIB, 0x00);
  probe_write(REG_OUT_MSB, 0x12);
  probe_write(REG_OUT_MSB + 2, 0x07);
  probe_write(REG_CTRL_MEAS - 1, 0x5a);
  probe_write(REG_CTRL_MEAS + 1, 0x5a);
#else
  Serial.println(F("  skipped"));
#endif
}

void q6_reads() {
  section(F("Q6 reads from reserved and write-only registers"));
  reset_sensor();

  label(F("soft_reset"));
  print_reg(read_reg(REG_SOFT_RESET));
  Serial.println();
  write_reg(REG_SOFT_RESET, 0x5a);
  label(F("soft_reset after writing 5a"));
  print_reg(read_reg(REG_SOFT_RESET));
  Serial.println();

  uint8_t buf[16];
  label(F("burst read from fe"));
  if (read_regs(0xfe, buf, 4)) {
    print_bytes(buf, 4);
    Serial.println();
  } else {
    Serial.println(F("read failed"));
  }

  subsection(F("register dump"));
  for (uint16_t row = 0; row < 0x100; row += 16) {
    Serial.print(F("    "));
    print_hex(row);
    Serial.print(F(": "));
    if (read_regs(row, buf, sizeof(buf))) {
      print_bytes(buf, sizeof(buf));
      Serial.println();
    } else {
      Serial.println(F("read failed"));
    }
  }
}

void q7_burst_writes() {
  section(F("Q7 burst writes"));
#if WRITE_PROTECTED_REGISTERS
  // Without auto-increment, both bytes land in ctrl_meas.
  reset_sensor();
  const uint8_t a[] = {MC_PRESSURE, CMD_TEMPERATURE};
  uint8_t status_a = write_regs(REG_CTRL_MEAS, a, sizeof(a));
  int16_t ctrl_a = read_reg(REG_CTRL_MEAS);
  wait_sco(micros());

  // With auto-increment, the second byte lands in ctrl_meas.
  reset_sensor();
  const uint8_t b[] = {0x00, CMD_TEMPERATURE};
  uint8_t status_b = write_regs(REG_CTRL_MEAS - 1, b, sizeof(b));
  int16_t ctrl_b = read_reg(REG_CTRL_MEAS);
  wait_sco(micros());

  label(F("f4 <- 14 2e"));
  print_status(status_a);
  Serial.print(F(", ctrl_meas "));
  print_reg(ctrl_a);
  Serial.println();
  label(F("f3 <- 00 2e"));
  print_status(status_b);
  Serial.print(F(", ctrl_meas "));
  print_reg(ctrl_b);
  Serial.println();

  // Bit 2 of ctrl_meas always reads as 0.
  bool a_first = ctrl_a >= 0 && (ctrl_a & ~CTRL_MEAS_UNUSED) == (MC_PRESSURE & ~CTRL_MEAS_UNUSED);
  bool a_second = ctrl_a >= 0 && (ctrl_a & CTRL_MEAS_MC & ~CTRL_MEAS_UNUSED) ==
                                     (MC_TEMPERATURE & ~CTRL_MEAS_UNUSED);
  bool b_started = ctrl_b >= 0 && (ctrl_b & CTRL_MEAS_MC & ~CTRL_MEAS_UNUSED) ==
                                      (MC_TEMPERATURE & ~CTRL_MEAS_UNUSED);
  label(F("verdict"));
  if (a_first && b_started) {
    Serial.println(F("address auto-increments"));
  } else if (a_second && !b_started) {
    Serial.println(F("all bytes go to the same register"));
  } else if (a_first && !b_started) {
    Serial.println(F("address/data pairs or extra bytes ignored"));
  } else {
    Serial.println(F("unclear"));
  }
#else
  Serial.println(F("  skipped"));
#endif
}

void q8_xlsb_after_temperature() {
  section(F("Q8 out_xlsb after temperature conversion"));
  reset_sensor();
  uint8_t out[3];
  run_conversion(F("temperature after reset"), CMD_TEMPERATURE, out);
  run_conversion(F("pressure oss3"), CMD_PRESSURE | OSS(3), out);
  run_conversion(F("temperature after pressure oss3"), CMD_TEMPERATURE, out);
}

void q9_data_during_conversion() {
  section(F("Q9 data registers during conversion"));
  reset_sensor();
  trace(F("temperature after reset"), CMD_TEMPERATURE);
  trace(F("pressure oss3 after temperature"), CMD_PRESSURE | OSS(3));
  trace(F("temperature after pressure"), CMD_TEMPERATURE);
}

void q10_noise() {
  section(F("Q10 noise"));
  reset_sensor();
  uint8_t out[3];

  stats ut, t;
  for (uint8_t i = 0; i < NOISE_SAMPLES; i++) {
    wait_sco(start(CMD_TEMPERATURE));
    read_regs(REG_OUT_MSB, out, 3);
    ut.add(raw_ut(out));
    t.add(compute_b5(raw_ut(out)) / 160.0);
  }

  label(F("temperature"));
  Serial.print(F("mean "));
  Serial.print(t.mean, 2);
  Serial.print(F(" C, rms "));
  Serial.print(t.rms(), 3);
  Serial.print(F(" C, ut rms "));
  Serial.println(ut.rms(), 2);

  // Compensate with a fixed temperature to isolate the pressure noise.
  long b5 = compute_b5(lround(ut.mean));
  for (uint8_t oss = 0; oss < 4; oss++) {
    stats up, p;
    for (uint8_t i = 0; i < NOISE_SAMPLES; i++) {
      wait_sco(start(CMD_PRESSURE | OSS(oss)));
      read_regs(REG_OUT_MSB, out, 3);
      up.add(raw_up(out, oss));
      p.add(compute_pressure(raw_up(out, oss), b5, oss));
    }

    Serial.print(F("    pressure oss"));
    Serial.print(oss);
    Serial.print(F(": mean "));
    Serial.print(p.mean / 100.0, 2);
    Serial.print(F(" hPa, rms "));
    Serial.print(p.rms(), 1);
    Serial.print(F(" Pa, up rms "));
    Serial.println(up.rms(), 2);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println(F("BMP180 edge cases"));
  Serial.print(F("  samples for noise: "));
  Serial.println(NOISE_SAMPLES);

  if (read_reg(REG_ID) != CHIP_ID) {
    Serial.println(F("BMP180 not found"));
    while (true) {
    }
  }

  if (!read_calib()) {
    Serial.println(F("reading calibration failed"));
    while (true) {
    }
  }

  q0_reference();
  q1_restart();
  q2_unknown_control();
  q3_oss_change();
  q4_reset_during_conversion();
  q5_protected_writes();
  q6_reads();
  q7_burst_writes();
  q8_xlsb_after_temperature();
  q9_data_during_conversion();
  q10_noise();

  reset_sensor();
  Serial.println();
  Serial.print(F("done after "));
  Serial.print(millis());
  Serial.println(F(" ms"));
}

void loop() {
}
