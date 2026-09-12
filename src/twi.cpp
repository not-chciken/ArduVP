#include "twi.h"

static constexpr u8 TWCR_TWINT = mwr::bit(7);
static constexpr u8 TWCR_TWEA = mwr::bit(6);
static constexpr u8 TWCR_TWSTA = mwr::bit(5);
static constexpr u8 TWCR_TWSTO = mwr::bit(4);
static constexpr u8 TWCR_TWWC = mwr::bit(3);
static constexpr u8 TWCR_TWEN = mwr::bit(2);
static constexpr u8 TWCR_TWIE = mwr::bit(0);

static constexpr u8 TWSR_TWPS_MASK = 0x03;

static constexpr u8 TWAR_TWGCE = mwr::bit(0);

static constexpr u8 TWS_IDLE = 0xf8;

static constexpr u8 TWS_MT_START = 0x08;
static constexpr u8 TWS_MT_REP_START = 0x10;
static constexpr u8 TWS_MT_SLA_ACK = 0x18;
static constexpr u8 TWS_MT_SLA_NACK = 0x20;
static constexpr u8 TWS_MT_DATA_ACK = 0x28;
static constexpr u8 TWS_MT_DATA_NACK = 0x30;

static constexpr u8 TWS_MR_SLA_ACK = 0x40;
static constexpr u8 TWS_MR_SLA_NACK = 0x48;
static constexpr u8 TWS_MR_DATA_ACK = 0x50;
static constexpr u8 TWS_MR_DATA_NACK = 0x58;

static constexpr u8 TWS_SR_SLA_ACK = 0x60;
static constexpr u8 TWS_SR_GCALL_ACK = 0x70;
static constexpr u8 TWS_SR_DATA_ACK = 0x80;
static constexpr u8 TWS_SR_DATA_NACK = 0x88;
static constexpr u8 TWS_SR_GCALL_DATA_ACK = 0x90;
static constexpr u8 TWS_SR_GCALL_DATA_NACK = 0x98;
static constexpr u8 TWS_SR_STOP = 0xa0;

static constexpr u8 TWS_ST_SLA_ACK = 0xa8;
static constexpr u8 TWS_ST_DATA_ACK = 0xb8;
static constexpr u8 TWS_ST_DATA_NACK = 0xc0;

Twi::Twi(const sc_core::sc_module_name& nm)
    : vcml::peripheral(nm),
      vcml::i2c_host(),
      twbr("twbr", 0xb8, 0x00),
      twcr("twcr", 0xbc, 0x00),
      twsr("twsr", 0xb9, TWS_IDLE),
      twdr("twdr", 0xbb, 0xff),
      twar("twar", 0xba, 0xfe),
      twamr("twamr", 0xbd, 0x00),
      out_i2c("out_i2c"),
      in_i2c("in_i2c"),
      irq("irq"),
      ack("ack"),
      in("in"),
      master_active_(false),
      master_start_pending_(false),
      master_dir_(vcml::TLM_IGNORE_COMMAND),
      slave_addressed_(false),
      slave_general_call_(false),
      slave_dir_(vcml::TLM_IGNORE_COMMAND) {
  set_little_endian();

  twbr.sync_always();
  twbr.allow_read_write();

  twcr.sync_always();
  twcr.allow_read_write();
  twcr.on_write(&Twi::WriteTwcr);

  twsr.sync_always();
  twsr.allow_read_write();
  twsr.on_write(&Twi::WriteTwsr);

  twdr.sync_always();
  twdr.allow_read_write();
  twdr.on_write(&Twi::WriteTwdr);

  twar.sync_always();
  twar.allow_read_write();
  twar.on_write(&Twi::WriteTwar);

  twamr.sync_always();
  twamr.allow_read_write();
  twamr.on_write(&Twi::WriteTwamr);
}

void Twi::reset() {
  peripheral::reset();

  master_active_ = false;
  master_start_pending_ = false;
  master_dir_ = vcml::TLM_IGNORE_COMMAND;

  slave_addressed_ = false;
  slave_general_call_ = false;
  slave_dir_ = vcml::TLM_IGNORE_COMMAND;

  irq = false;
}

void Twi::SetStatus(u8 code) {
  twsr = (u8)((twsr & TWSR_TWPS_MASK) | code);
  twcr = (u8)(twcr | TWCR_TWINT);
  UpdateIrq();
}

void Twi::UpdateIrq() {
  irq = (twcr & TWCR_TWINT) && (twcr & TWCR_TWIE);
}

void Twi::AbortTransaction() {
  if (master_active_)
    out_i2c.stop();

  master_active_ = false;
  master_start_pending_ = false;
  master_dir_ = vcml::TLM_IGNORE_COMMAND;

  slave_addressed_ = false;
  slave_general_call_ = false;
  slave_dir_ = vcml::TLM_IGNORE_COMMAND;

  twsr = (u8)((twsr & TWSR_TWPS_MASK) | TWS_IDLE);
}

void Twi::WriteTwsr(u8 val) {
  twsr = (u8)((twsr & ~TWSR_TWPS_MASK) | (val & TWSR_TWPS_MASK));
}

void Twi::WriteTwar(u8 val) {
  twar = val;
}

void Twi::WriteTwamr(u8 val) {
  twamr = (u8)(val & ~0x01);
}

void Twi::WriteTwdr(u8 val) {
  if (!(twcr & TWCR_TWINT)) {
    twcr = (u8)(twcr | TWCR_TWWC);
    return;
  }

  twcr = (u8)(twcr & ~TWCR_TWWC);
  twdr = val;
}

void Twi::WriteTwcr(u8 val) {
  bool twen = (val & TWCR_TWEN) != 0;
  bool twie = (val & TWCR_TWIE) != 0;
  bool twea = (val & TWCR_TWEA) != 0;
  bool twsta = (val & TWCR_TWSTA) != 0;
  bool twsto = (val & TWCR_TWSTO) != 0;
  bool twint = (val & TWCR_TWINT) != 0;

  u8 cfg = (u8)((twcr & TWCR_TWINT) | (twea ? TWCR_TWEA : 0) | (twsta ? TWCR_TWSTA : 0) | (twen ? TWCR_TWEN : 0) |
                (twie ? TWCR_TWIE : 0) | (twcr & TWCR_TWWC));

  if (!twen) {
    AbortTransaction();
    twcr = (u8)(cfg & ~TWCR_TWINT);
    irq = false;
    return;
  }

  twcr = cfg;

  if (!twint) {
    UpdateIrq();
    return;
  }

  twcr = (u8)(twcr & ~TWCR_TWINT);
  UpdateIrq();

  if (twsta) {
    if (twsto) {
      if (master_active_)
        out_i2c.stop();
      master_active_ = false;
      master_start_pending_ = false;
    }

    bool repeated = master_active_;
    master_active_ = true;
    master_start_pending_ = true;
    master_dir_ = vcml::TLM_IGNORE_COMMAND;
    slave_addressed_ = false;
    slave_general_call_ = false;

    SetStatus(repeated ? TWS_MT_REP_START : TWS_MT_START);
    return;
  }

  if (twsto) {
    if (master_active_)
      out_i2c.stop();
    master_active_ = false;
    master_start_pending_ = false;
    master_dir_ = vcml::TLM_IGNORE_COMMAND;

    twsr = (u8)((twsr & TWSR_TWPS_MASK) | TWS_IDLE);
    UpdateIrq();
    return;
  }

  if (master_active_) {
    DoMasterContinue(twea);
    return;
  }

  twsr = (u8)((twsr & TWSR_TWPS_MASK) | TWS_IDLE);
  UpdateIrq();
}

void Twi::DoMasterContinue(bool twea) {
  if (master_start_pending_) {
    u8 sla_rw = twdr;
    u8 addr = vcml::i2c_decode_address(sla_rw);
    vcml::tlm_command cmd = vcml::i2c_decode_tlm_command(sla_rw);

    vcml::i2c_response resp = out_i2c.start(addr, cmd);
    master_start_pending_ = false;

    bool is_read = cmd == vcml::TLM_READ_COMMAND;
    if (vcml::success(resp)) {
      master_dir_ = cmd;
      SetStatus(is_read ? TWS_MR_SLA_ACK : TWS_MT_SLA_ACK);
    } else {
      master_dir_ = vcml::TLM_IGNORE_COMMAND;
      SetStatus(is_read ? TWS_MR_SLA_NACK : TWS_MT_SLA_NACK);
    }
    return;
  }

  if (master_dir_ == vcml::TLM_WRITE_COMMAND) {
    u8 data = twdr;
    vcml::i2c_response resp = out_i2c.transport(data);
    SetStatus(vcml::success(resp) ? TWS_MT_DATA_ACK : TWS_MT_DATA_NACK);
  } else {
    u8 data = 0;
    out_i2c.transport(data);
    twdr = data;
    SetStatus(twea ? TWS_MR_DATA_ACK : TWS_MR_DATA_NACK);
  }
}

void Twi::i2c_transport(vcml::i2c_target_socket& socket, vcml::i2c_payload& tx) {
  if (!(twcr & TWCR_TWEN)) {
    tx.resp = vcml::I2C_NACK;
    return;
  }

  switch (tx.cmd) {
  case vcml::I2C_START:
    SlaveStart(tx);
    break;
  case vcml::I2C_DATA:
    SlaveData(tx);
    break;
  case vcml::I2C_STOP:
    SlaveStop(tx);
    break;
  default:
    tx.resp = vcml::I2C_NACK;
    break;
  }
}

void Twi::SlaveStart(vcml::i2c_payload& tx) {
  bool twea = (twcr & TWCR_TWEA) != 0;
  u8 addr = vcml::i2c_decode_address(tx.data);
  bool is_read = vcml::i2c_decode_tlm_command(tx.data) == vcml::TLM_READ_COMMAND;

  u8 own = (u8)((twar >> 1) & 0x7f);
  u8 mask = (u8)((twamr >> 1) & 0x7f);
  bool own_match = twea && (((addr ^ own) & (u8)(~mask & 0x7f)) == 0);
  bool gc_match = twea && !is_read && (twar & TWAR_TWGCE) && addr == 0;

  if (!own_match && !gc_match) {
    tx.resp = vcml::I2C_NACK;
    slave_addressed_ = false;
    return;
  }

  slave_addressed_ = true;
  slave_general_call_ = gc_match && !own_match;
  slave_dir_ = is_read ? vcml::TLM_READ_COMMAND : vcml::TLM_WRITE_COMMAND;

  u8 status = is_read ? TWS_ST_SLA_ACK : (slave_general_call_ ? TWS_SR_GCALL_ACK : TWS_SR_SLA_ACK);
  SetStatus(status);
  tx.resp = vcml::I2C_ACK;
}

void Twi::SlaveData(vcml::i2c_payload& tx) {
  if (!slave_addressed_) {
    tx.resp = vcml::I2C_NACK;
    return;
  }

  bool twea = (twcr & TWCR_TWEA) != 0;

  if (slave_dir_ == vcml::TLM_WRITE_COMMAND) {
    twdr = tx.data;
    u8 status = slave_general_call_ ? (twea ? TWS_SR_GCALL_DATA_ACK : TWS_SR_GCALL_DATA_NACK)
                                    : (twea ? TWS_SR_DATA_ACK : TWS_SR_DATA_NACK);
    SetStatus(status);
    tx.resp = twea ? vcml::I2C_ACK : vcml::I2C_NACK;
  } else {
    tx.data = twdr;
    SetStatus(twea ? TWS_ST_DATA_ACK : TWS_ST_DATA_NACK);
    tx.resp = twea ? vcml::I2C_ACK : vcml::I2C_NACK;
  }
}

void Twi::SlaveStop(vcml::i2c_payload& tx) {
  bool was_addressed = slave_addressed_;

  slave_addressed_ = false;
  slave_general_call_ = false;
  slave_dir_ = vcml::TLM_IGNORE_COMMAND;

  if (was_addressed)
    SetStatus(TWS_SR_STOP);

  tx.resp = vcml::I2C_ACK;
}
