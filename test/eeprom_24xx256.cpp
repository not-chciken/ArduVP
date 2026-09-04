#include "eeprom_24xx256.h"

#include "testing.h"

struct eeprom_24xx256_test : public test_base, i2c_host {
  static constexpr u8 I2C_ADDR = 0x50;
  static constexpr u8 I2C_ADDR_CS = 0x57;
  static constexpr u16 EEPROM_SIZE = 0x8000;
  static constexpr u16 PAGE_SIZE = 64;
  static constexpr u8 ERASED = 0xff;

  i2c::eeprom_24xx256 eeprom;
  i2c::eeprom_24xx256 eeprom_cs;
  i2c_initiator_socket socket;
  i2c_response resp;
  u8 data;

  eeprom_24xx256_test(const sc_module_name& nm)
      : test_base(nm), eeprom("eeprom"), eeprom_cs("eeprom_cs", 0b111u), socket("socket") {
    socket.bind(eeprom.i2c_in);
    socket.bind(eeprom_cs.i2c_in);
    rst.bind(eeprom.rst);
    rst.bind(eeprom_cs.rst);

    add_test("address", &eeprom_24xx256_test::test_address);
    add_test("write_0_byte", &eeprom_24xx256_test::test_write_0_byte);
    add_test("write_1_byte", &eeprom_24xx256_test::test_write_1_byte);
    add_test("write_n_bytes_in_page", &eeprom_24xx256_test::test_write_n_bytes_in_page);
    add_test("write_n_bytes_crossing_pages", &eeprom_24xx256_test::test_write_n_bytes_crossing_pages);
    add_test("write_65_bytes", &eeprom_24xx256_test::test_write_65_bytes);
    add_test("write_during_write", &eeprom_24xx256_test::test_write_during_write);
    add_test("write_protected", &eeprom_24xx256_test::test_write_protected);
    add_test("read_0_byte", &eeprom_24xx256_test::test_read_0_byte);
    add_test("read_1_byte", &eeprom_24xx256_test::test_read_1_byte);
    add_test("read_n_bytes_in_page", &eeprom_24xx256_test::test_read_n_bytes_in_page);
    add_test("read_n_bytes_crossing_pages", &eeprom_24xx256_test::test_read_n_bytes_crossing_pages);
    add_test("read_0x8001_bytes", &eeprom_24xx256_test::test_read_0x8001_bytes);
    add_test("read_during_write", &eeprom_24xx256_test::test_read_during_write);
  }

  sc_time write_cycle_time() const { return sc_time(5.0, SC_MS); }
  sc_time poll_interval() const { return sc_time(100.0, SC_US); }

  u8& mem(u16 addr) { return eeprom.get_data()[addr]; }

  void mem_fill(u16 addr, size_t len, u8 value) {
    for (size_t i = 0; i < len; i++)
      mem((addr + i) % EEPROM_SIZE) = value;
  }

  void mem_pattern(u16 addr, size_t len, u8 first) {
    for (size_t i = 0; i < len; i++)
      mem((addr + i) % EEPROM_SIZE) = first + i;
  }

  void set_address(u16 addr) {
    ASSERT_EQ(socket.start(I2C_ADDR, tlm::TLM_WRITE_COMMAND), I2C_ACK);
    data = addr >> 8;
    ASSERT_EQ(socket.transport(data), I2C_ACK);
    data = addr & 0xff;
    ASSERT_EQ(socket.transport(data), I2C_ACK);
  }

  void write(u16 addr, const vector<u8>& bytes) {
    set_address(addr);

    for (u8 byte : bytes) {
      data = byte;
      ASSERT_EQ(socket.transport(data), I2C_ACK);
    }

    ASSERT_EQ(socket.stop(), I2C_ACK);
  }

  sc_time poll_until_ready() {
    sc_time start = sc_time_stamp();

    while (socket.start(I2C_ADDR, tlm::TLM_WRITE_COMMAND) != I2C_ACK)
      wait(poll_interval());

    EXPECT_EQ(socket.stop(), I2C_ACK);
    return sc_time_stamp() - start;
  }

  void await_write_cycle() {
    sc_time elapsed = poll_until_ready();
    ASSERT_GE(elapsed, write_cycle_time());
    ASSERT_LE(elapsed, write_cycle_time() + poll_interval());
  }

  vector<u8> read_current(size_t n) {
    vector<u8> bytes;
    EXPECT_EQ(socket.start(I2C_ADDR, tlm::TLM_READ_COMMAND), I2C_ACK);

    for (size_t i = 0; i < n; i++) {
      data = 0;
      EXPECT_EQ(socket.transport(data), I2C_ACK);
      bytes.push_back(data);
    }

    EXPECT_EQ(socket.stop(), I2C_ACK);
    return bytes;
  }

  vector<u8> read_at(u16 addr, size_t n) {
    set_address(addr);
    return read_current(n);
  }

  void test_address() {
    for (u8 addr = 0; addr < 128; addr++) {
      bool addressed = addr == I2C_ADDR || addr == I2C_ADDR_CS || addr == I2C_ADDR_BCAST;
      resp = socket.start(addr, tlm::TLM_WRITE_COMMAND);
      ASSERT_EQ(resp, addressed ? I2C_ACK : I2C_NACK) << "address " << (unsigned)addr;

      if (addressed) {
        ASSERT_EQ(socket.stop(), I2C_ACK);
      }
    }

    ASSERT_EQ(socket.start(I2C_ADDR, tlm::TLM_READ_COMMAND), I2C_ACK);
    ASSERT_EQ(socket.stop(), I2C_ACK);

    mem_fill(0x0000, 1, ERASED);
    ASSERT_EQ(socket.start(I2C_ADDR_CS, tlm::TLM_WRITE_COMMAND), I2C_ACK);
    data = 0x00;
    ASSERT_EQ(socket.transport(data), I2C_ACK);
    ASSERT_EQ(socket.transport(data), I2C_ACK);
    data = 0x5a;
    ASSERT_EQ(socket.transport(data), I2C_ACK);
    ASSERT_EQ(socket.stop(), I2C_ACK);
    wait(write_cycle_time() + poll_interval());
    ASSERT_EQ(mem(0x0000), ERASED);
    ASSERT_EQ(eeprom_cs.get_data()[0x0000], 0x5a);
  }

  void test_write_0_byte() {
    const u16 addr = 0x0010;
    mem_fill(addr, 1, 0x5a);

    set_address(addr);
    ASSERT_EQ(socket.stop(), I2C_ACK);

    ASSERT_EQ(socket.start(I2C_ADDR, tlm::TLM_WRITE_COMMAND), I2C_ACK);
    ASSERT_EQ(socket.stop(), I2C_ACK);
    ASSERT_EQ(mem(addr), 0x5a);

    ASSERT_EQ(read_current(1), vector<u8>({0x5a}));
  }

  void test_write_1_byte() {
    const u16 addr = 0x1234;
    mem_fill(addr - 1, 3, ERASED);

    write(addr, {0x42});

    ASSERT_EQ(mem(addr), ERASED);
    ASSERT_EQ(socket.start(I2C_ADDR, tlm::TLM_WRITE_COMMAND), I2C_NACK);

    await_write_cycle();

    ASSERT_EQ(mem(addr), 0x42);
    ASSERT_EQ(mem(addr - 1), ERASED);
    ASSERT_EQ(mem(addr + 1), ERASED);
    ASSERT_EQ(read_at(addr, 1), vector<u8>({0x42}));
  }

  void test_write_n_bytes_in_page() {
    const u16 page = 0x0100;
    const u16 addr = page + 4;
    vector<u8> bytes;
    for (u8 i = 0; i < 16; i++)
      bytes.push_back(0xa0 + i);

    mem_fill(page, PAGE_SIZE, ERASED);
    write(addr, bytes);
    await_write_cycle();

    for (size_t i = 0; i < bytes.size(); i++)
      ASSERT_EQ(mem(addr + i), bytes[i]) << "byte " << i;

    ASSERT_EQ(mem(addr - 1), ERASED);
    ASSERT_EQ(mem(addr + bytes.size()), ERASED);
    ASSERT_EQ(read_at(addr, bytes.size()), bytes);
  }

  void test_write_n_bytes_crossing_pages() {
    const u16 page = 0x0080;
    const u16 addr = page + PAGE_SIZE - 4;
    vector<u8> bytes = {1, 2, 3, 4, 5, 6, 7, 8};

    mem_fill(page, 2 * PAGE_SIZE, ERASED);
    write(addr, bytes);
    await_write_cycle();

    for (size_t i = 0; i < 4; i++)
      ASSERT_EQ(mem(addr + i), bytes[i]) << "byte " << i;
    for (size_t i = 4; i < bytes.size(); i++)
      ASSERT_EQ(mem(page + i - 4), bytes[i]) << "byte " << i;

    for (u16 i = 0; i < PAGE_SIZE; i++)
      ASSERT_EQ(mem(page + PAGE_SIZE + i), ERASED) << "byte " << i;

    ASSERT_EQ(read_at(addr, 4), vector<u8>({1, 2, 3, 4}));
    ASSERT_EQ(read_at(page, 4), vector<u8>({5, 6, 7, 8}));
  }

  void test_write_65_bytes() {
    const u16 page = 0x0200;
    vector<u8> bytes;
    for (size_t i = 0; i <= PAGE_SIZE; i++)
      bytes.push_back(i);

    mem_fill(page, 2 * PAGE_SIZE, ERASED);
    write(page, bytes);
    await_write_cycle();

    ASSERT_EQ(mem(page), bytes[PAGE_SIZE]);
    for (u16 i = 1; i < PAGE_SIZE; i++)
      ASSERT_EQ(mem(page + i), bytes[i]) << "byte " << i;
    for (u16 i = 0; i < PAGE_SIZE; i++)
      ASSERT_EQ(mem(page + PAGE_SIZE + i), ERASED) << "byte " << i;
  }

  void test_write_during_write() {
    const u16 addr = 0x0300;
    const u16 other = 0x0400;
    mem_fill(addr, 1, ERASED);
    mem_fill(other, 1, ERASED);

    write(addr, {0x11});
    wait(write_cycle_time() / 2);

    ASSERT_EQ(socket.start(I2C_ADDR, tlm::TLM_WRITE_COMMAND), I2C_NACK);
    data = other >> 8;
    ASSERT_EQ(socket.transport(data), I2C_NACK);
    data = other & 0xff;
    ASSERT_EQ(socket.transport(data), I2C_NACK);
    data = 0x22;
    ASSERT_EQ(socket.transport(data), I2C_NACK);
    ASSERT_EQ(socket.stop(), I2C_NACK);

    sc_time elapsed = poll_until_ready();
    ASSERT_LE(elapsed, write_cycle_time() / 2 + poll_interval());
    ASSERT_EQ(mem(addr), 0x11);
    ASSERT_EQ(mem(other), ERASED);

    write(other, {0x22});
    await_write_cycle();
    ASSERT_EQ(mem(other), 0x22);
  }

  void test_write_protected() {
    const u16 addr = 0x0a00;
    mem_fill(addr, 2, ERASED);

    eeprom.wp = true;
    write(addr, {0x55, 0x66});

    ASSERT_EQ(socket.start(I2C_ADDR, tlm::TLM_WRITE_COMMAND), I2C_ACK);
    ASSERT_EQ(socket.stop(), I2C_ACK);

    wait(write_cycle_time() + poll_interval());
    ASSERT_EQ(mem(addr), ERASED);
    ASSERT_EQ(mem(addr + 1), ERASED);

    mem_pattern(addr, 2, 0x90);
    ASSERT_EQ(read_current(2), vector<u8>({0x90, 0x91}));

    eeprom.wp = false;
    write(addr, {0x55, 0x66});
    await_write_cycle();
    ASSERT_EQ(read_at(addr, 2), vector<u8>({0x55, 0x66}));
  }

  void test_read_0_byte() {
    const u16 addr = 0x0500;
    mem_pattern(addr, 2, 0x70);

    set_address(addr);
    ASSERT_EQ(socket.start(I2C_ADDR, tlm::TLM_READ_COMMAND), I2C_ACK);
    ASSERT_EQ(socket.stop(), I2C_ACK);

    ASSERT_EQ(read_current(1), vector<u8>({0x70}));
  }

  void test_read_1_byte() {
    const u16 addr = 0x0600;
    mem_pattern(addr, 2, 0x80);

    ASSERT_EQ(read_at(addr, 1), vector<u8>({0x80}));
    ASSERT_EQ(read_current(1), vector<u8>({0x81}));
  }

  void test_read_n_bytes_in_page() {
    const u16 addr = 0x0704;
    vector<u8> expected;
    for (u8 i = 0; i < 16; i++)
      expected.push_back(0x10 + i);

    mem_pattern(addr, expected.size(), 0x10);
    ASSERT_EQ(read_at(addr, expected.size()), expected);
  }

  void test_read_n_bytes_crossing_pages() {
    u16 addr = 0x0800 + PAGE_SIZE - 4;
    vector<u8> expected;
    for (u8 i = 0; i < 8; i++)
      expected.push_back(0x30 + i);

    mem_pattern(addr, expected.size(), 0x30);
    ASSERT_EQ(read_at(addr, expected.size()), expected);
  }

  void test_read_0x8001_bytes() {
    for (size_t addr = 0; addr < EEPROM_SIZE; addr++)
      mem(addr) = (u8)(addr ^ (addr >> 8) ^ 0xa5);

    vector<u8> bytes = read_at(0x0000, EEPROM_SIZE + 1);
    ASSERT_EQ(bytes.size(), (size_t)EEPROM_SIZE + 1);

    for (size_t addr = 0; addr < EEPROM_SIZE; addr++) {
      if (bytes[addr] != mem(addr))
        FAIL() << "unexpected data at address " << addr;
    }

    ASSERT_EQ(bytes[EEPROM_SIZE], mem(0x0000));
  }

  void test_read_during_write() {
    const u16 addr = 0x0900;
    mem_fill(addr, 1, ERASED);

    write(addr, {0x33});

    ASSERT_EQ(socket.start(I2C_ADDR, tlm::TLM_READ_COMMAND), I2C_NACK);
    data = 0x00;
    ASSERT_EQ(socket.transport(data), I2C_NACK);
    ASSERT_EQ(data, 0x00);
    ASSERT_EQ(socket.stop(), I2C_NACK);

    await_write_cycle();
    ASSERT_EQ(read_at(addr, 1), vector<u8>({0x33}));
  }
};

TEST(i2c, eeprom_24xx256_test) {
  eeprom_24xx256_test test("test");
  sc_core::sc_start();
}
