#include "cpu6510.h"
#include "SIDBlasterUtils.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

// Initialize the opcode table
const std::array<OpcodeInfo, 256> CPU6510::opcodeTable_ = { {

        // 0x00
        {Instruction::BRK, "brk", AddressingMode::Implied, 7, false},
        {Instruction::ORA, "ora", AddressingMode::IndirectX, 6, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::SLO, "slo", AddressingMode::IndirectX, 8, true},
        {Instruction::NOP, "nop", AddressingMode::ZeroPage, 3, true},
        {Instruction::ORA, "ora", AddressingMode::ZeroPage, 3, false},
        {Instruction::ASL, "asl", AddressingMode::ZeroPage, 5, false},
        {Instruction::SLO, "slo", AddressingMode::ZeroPage, 5, true},
        {Instruction::PHP, "php", AddressingMode::Implied, 3, false},
        {Instruction::ORA, "ora", AddressingMode::Immediate, 2, false},
        {Instruction::ASL, "asl", AddressingMode::Accumulator, 2, false},
        {Instruction::ANC, "anc", AddressingMode::Immediate, 2, true},
        {Instruction::NOP, "nop", AddressingMode::Absolute, 4, true},
        {Instruction::ORA, "ora", AddressingMode::Absolute, 4, false},
        {Instruction::ASL, "asl", AddressingMode::Absolute, 6, false},
        {Instruction::SLO, "slo", AddressingMode::Absolute, 6, true},

        // 0x10
        {Instruction::BPL, "bpl", AddressingMode::Relative, 2, false},
        {Instruction::ORA, "ora", AddressingMode::IndirectY, 5, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::SLO, "slo", AddressingMode::IndirectY, 8, true},
        {Instruction::NOP, "nop", AddressingMode::ZeroPageX, 4, true},
        {Instruction::ORA, "ora", AddressingMode::ZeroPageX, 4, false},
        {Instruction::ASL, "asl", AddressingMode::ZeroPageX, 6, false},
        {Instruction::SLO, "slo", AddressingMode::ZeroPageX, 6, true},
        {Instruction::CLC, "clc", AddressingMode::Implied, 2, false},
        {Instruction::ORA, "ora", AddressingMode::AbsoluteY, 4, false},
        {Instruction::NOP, "nop", AddressingMode::Implied, 2, true},
        {Instruction::SLO, "slo", AddressingMode::AbsoluteY, 7, true},
        {Instruction::NOP, "nop", AddressingMode::AbsoluteX, 4, true},
        {Instruction::ORA, "ora", AddressingMode::AbsoluteX, 4, false},
        {Instruction::ASL, "asl", AddressingMode::AbsoluteX, 7, false},
        {Instruction::SLO, "slo", AddressingMode::AbsoluteX, 7, true},

        // 0x20
        {Instruction::JSR, "jsr", AddressingMode::Absolute, 6, false},
        {Instruction::AND, "and", AddressingMode::IndirectX, 6, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::RLA, "rla", AddressingMode::IndirectX, 8, true},
        {Instruction::BIT, "bit", AddressingMode::ZeroPage, 3, false},
        {Instruction::AND, "and", AddressingMode::ZeroPage, 3, false},
        {Instruction::ROL, "rol", AddressingMode::ZeroPage, 5, false},
        {Instruction::RLA, "rla", AddressingMode::ZeroPage, 5, true},
        {Instruction::PLP, "plp", AddressingMode::Implied, 4, false},
        {Instruction::AND, "and", AddressingMode::Immediate, 2, false},
        {Instruction::ROL, "rol", AddressingMode::Accumulator, 2, false},
        {Instruction::ANC, "anc", AddressingMode::Immediate, 2, true},
        {Instruction::BIT, "bit", AddressingMode::Absolute, 4, false},
        {Instruction::AND, "and", AddressingMode::Absolute, 4, false},
        {Instruction::ROL, "rol", AddressingMode::Absolute, 6, false},
        {Instruction::RLA, "rla", AddressingMode::Absolute, 6, true},

        // 0x30
        {Instruction::BMI, "bmi", AddressingMode::Relative, 2, false},
        {Instruction::AND, "and", AddressingMode::IndirectY, 5, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::RLA, "rla", AddressingMode::IndirectY, 8, true},
        {Instruction::NOP, "nop", AddressingMode::ZeroPageX, 4, true},
        {Instruction::AND, "and", AddressingMode::ZeroPageX, 4, false},
        {Instruction::ROL, "rol", AddressingMode::ZeroPageX, 6, false},
        {Instruction::RLA, "rla", AddressingMode::ZeroPageX, 6, true},
        {Instruction::SEC, "sec", AddressingMode::Implied, 2, false},
        {Instruction::AND, "and", AddressingMode::AbsoluteY, 4, false},
        {Instruction::NOP, "nop", AddressingMode::Implied, 2, true},
        {Instruction::RLA, "rla", AddressingMode::AbsoluteY, 7, true},
        {Instruction::NOP, "nop", AddressingMode::AbsoluteX, 4, true},
        {Instruction::AND, "and", AddressingMode::AbsoluteX, 4, false},
        {Instruction::ROL, "rol", AddressingMode::AbsoluteX, 7, false},
        {Instruction::RLA, "rla", AddressingMode::AbsoluteX, 7, true},

        // 0x40
        {Instruction::RTI, "rti", AddressingMode::Implied, 6, false},
        {Instruction::EOR, "eor", AddressingMode::IndirectX, 6, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::SRE, "sre", AddressingMode::IndirectX, 8, true},
        {Instruction::NOP, "nop", AddressingMode::ZeroPage, 3, true},
        {Instruction::EOR, "eor", AddressingMode::ZeroPage, 3, false},
        {Instruction::LSR, "lsr", AddressingMode::ZeroPage, 5, false},
        {Instruction::SRE, "sre", AddressingMode::ZeroPage, 5, true},
        {Instruction::PHA, "pha", AddressingMode::Implied, 3, false},
        {Instruction::EOR, "eor", AddressingMode::Immediate, 2, false},
        {Instruction::LSR, "lsr", AddressingMode::Accumulator, 2, false},
        {Instruction::ALR, "alr", AddressingMode::Immediate, 2, true},
        {Instruction::JMP, "jmp", AddressingMode::Absolute, 3, false},
        {Instruction::EOR, "eor", AddressingMode::Absolute, 4, false},
        {Instruction::LSR, "lsr", AddressingMode::Absolute, 6, false},
        {Instruction::SRE, "sre", AddressingMode::Absolute, 6, true},

        // 0x50
        {Instruction::BVC, "bvc", AddressingMode::Relative, 2, false},
        {Instruction::EOR, "eor", AddressingMode::IndirectY, 5, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::SRE, "sre", AddressingMode::IndirectY, 8, true},
        {Instruction::NOP, "nop", AddressingMode::ZeroPageX, 4, true},
        {Instruction::EOR, "eor", AddressingMode::ZeroPageX, 4, false},
        {Instruction::LSR, "lsr", AddressingMode::ZeroPageX, 6, false},
        {Instruction::SRE, "sre", AddressingMode::ZeroPageX, 6, true},
        {Instruction::CLI, "cli", AddressingMode::Implied, 2, false},
        {Instruction::EOR, "eor", AddressingMode::AbsoluteY, 4, false},
        {Instruction::NOP, "nop", AddressingMode::Implied, 2, true},
        {Instruction::SRE, "sre", AddressingMode::AbsoluteY, 7, true},
        {Instruction::NOP, "nop", AddressingMode::AbsoluteX, 4, true},
        {Instruction::EOR, "eor", AddressingMode::AbsoluteX, 4, false},
        {Instruction::LSR, "lsr", AddressingMode::AbsoluteX, 7, false},
        {Instruction::SRE, "sre", AddressingMode::AbsoluteX, 7, true},

        // 0x60
        {Instruction::RTS, "rts", AddressingMode::Implied, 6, false},
        {Instruction::ADC, "adc", AddressingMode::IndirectX, 6, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::RRA, "rra", AddressingMode::IndirectX, 8, true},
        {Instruction::NOP, "nop", AddressingMode::ZeroPage, 3, true},
        {Instruction::ADC, "adc", AddressingMode::ZeroPage, 3, false},
        {Instruction::ROR, "ror", AddressingMode::ZeroPage, 5, false},
        {Instruction::RRA, "rra", AddressingMode::ZeroPage, 5, true},
        {Instruction::PLA, "pla", AddressingMode::Implied, 4, false},
        {Instruction::ADC, "adc", AddressingMode::Immediate, 2, false},
        {Instruction::ROR, "ror", AddressingMode::Accumulator, 2, false},
        {Instruction::ARR, "arr", AddressingMode::Immediate, 2, true},
        {Instruction::JMP, "jmp", AddressingMode::Indirect, 5, false},
        {Instruction::ADC, "adc", AddressingMode::Absolute, 4, false},
        {Instruction::ROR, "ror", AddressingMode::Absolute, 6, false},
        {Instruction::RRA, "rra", AddressingMode::Absolute, 6, true},

        // 0x70
        {Instruction::BVS, "bvs", AddressingMode::Relative, 2, false},
        {Instruction::ADC, "adc", AddressingMode::IndirectY, 5, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::RRA, "rra", AddressingMode::IndirectY, 8, true},
        {Instruction::NOP, "nop", AddressingMode::ZeroPageX, 4, true},
        {Instruction::ADC, "adc", AddressingMode::ZeroPageX, 4, false},
        {Instruction::ROR, "ror", AddressingMode::ZeroPageX, 6, false},
        {Instruction::RRA, "rra", AddressingMode::ZeroPageX, 6, true},
        {Instruction::SEI, "sei", AddressingMode::Implied, 2, false},
        {Instruction::ADC, "adc", AddressingMode::AbsoluteY, 4, false},
        {Instruction::NOP, "nop", AddressingMode::Implied, 2, true},
        {Instruction::RRA, "rra", AddressingMode::AbsoluteY, 7, true},
        {Instruction::NOP, "nop", AddressingMode::AbsoluteX, 4, true},
        {Instruction::ADC, "adc", AddressingMode::AbsoluteX, 4, false},
        {Instruction::ROR, "ror", AddressingMode::AbsoluteX, 7, false},
        {Instruction::RRA, "rra", AddressingMode::AbsoluteX, 7, true},

        // 0x80
        {Instruction::NOP, "nop", AddressingMode::Immediate, 2, true},
        {Instruction::STA, "sta", AddressingMode::IndirectX, 6, false},
        {Instruction::NOP, "nop", AddressingMode::Immediate, 2, true},
        {Instruction::SAX, "sax", AddressingMode::IndirectX, 6, true},
        {Instruction::STY, "sty", AddressingMode::ZeroPage, 3, false},
        {Instruction::STA, "sta", AddressingMode::ZeroPage, 3, false},
        {Instruction::STX, "stx", AddressingMode::ZeroPage, 3, false},
        {Instruction::SAX, "sax", AddressingMode::ZeroPage, 3, true},
        {Instruction::DEY, "dey", AddressingMode::Implied, 2, false},
        {Instruction::NOP, "nop", AddressingMode::Immediate, 2, false},
        {Instruction::TXA, "txa", AddressingMode::Implied, 2, false},
        {Instruction::XAA, "xaa", AddressingMode::Immediate, 2, true},
        {Instruction::STY, "sty", AddressingMode::Absolute, 4, false},
        {Instruction::STA, "sta", AddressingMode::Absolute, 4, false},
        {Instruction::STX, "stx", AddressingMode::Absolute, 4, false},
        {Instruction::SAX, "sax", AddressingMode::Absolute, 4, true},

        // 0x90
        {Instruction::BCC, "bcc", AddressingMode::Relative, 2, false},
        {Instruction::STA, "sta", AddressingMode::IndirectY, 6, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::AHX, "ahx", AddressingMode::IndirectY, 6, true},
        {Instruction::STY, "sty", AddressingMode::ZeroPageX, 4, false},
        {Instruction::STA, "sta", AddressingMode::ZeroPageX, 4, false},
        {Instruction::STX, "stx", AddressingMode::ZeroPageY, 4, false},
        {Instruction::SAX, "sax", AddressingMode::ZeroPageY, 4, true},
        {Instruction::TYA, "tya", AddressingMode::Implied, 2, false},
        {Instruction::STA, "sta", AddressingMode::AbsoluteY, 5, false},
        {Instruction::TXS, "txs", AddressingMode::Implied, 2, false},
        {Instruction::TAS, "tas", AddressingMode::AbsoluteY, 5, true},
        {Instruction::SHY, "shy", AddressingMode::AbsoluteX, 5, true},
        {Instruction::STA, "sta", AddressingMode::AbsoluteX, 5, false},
        {Instruction::SHX, "shx", AddressingMode::AbsoluteY, 5, true},
        {Instruction::AHX, "ahx", AddressingMode::AbsoluteY, 5, true},

        // 0xA0
        {Instruction::LDY, "ldy", AddressingMode::Immediate, 2, false},
        {Instruction::LDA, "lda", AddressingMode::IndirectX, 6, false},
        {Instruction::LDX, "ldx", AddressingMode::Immediate, 2, false},
        {Instruction::LAX, "lax", AddressingMode::IndirectX, 6, true},
        {Instruction::LDY, "ldy", AddressingMode::ZeroPage, 3, false},
        {Instruction::LDA, "lda", AddressingMode::ZeroPage, 3, false},
        {Instruction::LDX, "ldx", AddressingMode::ZeroPage, 3, false},
        {Instruction::LAX, "lax", AddressingMode::ZeroPage, 3, true},
        {Instruction::TAY, "tay", AddressingMode::Implied, 2, false},
        {Instruction::LDA, "lda", AddressingMode::Immediate, 2, false},
        {Instruction::TAX, "tax", AddressingMode::Implied, 2, false},
        {Instruction::LAX, "lax", AddressingMode::Immediate, 2, true},
        {Instruction::LDY, "ldy", AddressingMode::Absolute, 4, false},
        {Instruction::LDA, "lda", AddressingMode::Absolute, 4, false},
        {Instruction::LDX, "ldx", AddressingMode::Absolute, 4, false},
        {Instruction::LAX, "lax", AddressingMode::Absolute, 4, true},

        // 0xB0
        {Instruction::BCS, "bcs", AddressingMode::Relative, 2, false},
        {Instruction::LDA, "lda", AddressingMode::IndirectY, 5, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::LAX, "lax", AddressingMode::IndirectY, 5, true},
        {Instruction::LDY, "ldy", AddressingMode::ZeroPageX, 4, false},
        {Instruction::LDA, "lda", AddressingMode::ZeroPageX, 4, false},
        {Instruction::LDX, "ldx", AddressingMode::ZeroPageY, 4, false},
        {Instruction::LAX, "lax", AddressingMode::ZeroPageY, 4, true},
        {Instruction::CLV, "clv", AddressingMode::Implied, 2, false},
        {Instruction::LDA, "lda", AddressingMode::AbsoluteY, 4, false},
        {Instruction::TSX, "tsx", AddressingMode::Implied, 2, false},
        {Instruction::LAS, "las", AddressingMode::AbsoluteY, 4, true},
        {Instruction::LDY, "ldy", AddressingMode::AbsoluteX, 4, false},
        {Instruction::LDA, "lda", AddressingMode::AbsoluteX, 4, false},
        {Instruction::LDX, "ldx", AddressingMode::AbsoluteY, 4, false},
        {Instruction::LAX, "lax", AddressingMode::AbsoluteY, 4, true},

        // 0xC0
        {Instruction::CPY, "cpy", AddressingMode::Immediate, 2, false},
        {Instruction::CMP, "cmp", AddressingMode::IndirectX, 6, false},
        {Instruction::NOP, "nop", AddressingMode::Immediate, 2, true},
        {Instruction::DCP, "dcp", AddressingMode::IndirectX, 8, true},
        {Instruction::CPY, "cpy", AddressingMode::ZeroPage, 3, false},
        {Instruction::CMP, "cmp", AddressingMode::ZeroPage, 3, false},
        {Instruction::DEC, "dec", AddressingMode::ZeroPage, 5, false},
        {Instruction::DCP, "dcp", AddressingMode::ZeroPage, 5, true},
        {Instruction::INY, "iny", AddressingMode::Implied, 2, false},
        {Instruction::CMP, "cmp", AddressingMode::Immediate, 2, false},
        {Instruction::DEX, "dex", AddressingMode::Implied, 2, false},
        {Instruction::AXS, "axs", AddressingMode::Immediate, 2, true},
        {Instruction::CPY, "cpy", AddressingMode::Absolute, 4, false},
        {Instruction::CMP, "cmp", AddressingMode::Absolute, 4, false},
        {Instruction::DEC, "dec", AddressingMode::Absolute, 6, false},
        {Instruction::DCP, "dcp", AddressingMode::Absolute, 6, true},

        // 0xD0
        {Instruction::BNE, "bne", AddressingMode::Relative, 2, false},
        {Instruction::CMP, "cmp", AddressingMode::IndirectY, 5, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::DCP, "dcp", AddressingMode::IndirectY, 8, true},
        {Instruction::NOP, "nop", AddressingMode::ZeroPageX, 4, true},
        {Instruction::CMP, "cmp", AddressingMode::ZeroPageX, 4, false},
        {Instruction::DEC, "dec", AddressingMode::ZeroPageX, 6, false},
        {Instruction::DCP, "dcp", AddressingMode::ZeroPageX, 6, true},
        {Instruction::CLD, "cld", AddressingMode::Implied, 2, false},
        {Instruction::CMP, "cmp", AddressingMode::AbsoluteY, 4, false},
        {Instruction::NOP, "nop", AddressingMode::Implied, 2, true},
        {Instruction::DCP, "dcp", AddressingMode::AbsoluteY, 7, true},
        {Instruction::NOP, "nop", AddressingMode::AbsoluteX, 4, true},
        {Instruction::CMP, "cmp", AddressingMode::AbsoluteX, 4, false},
        {Instruction::DEC, "dec", AddressingMode::AbsoluteX, 7, false},
        {Instruction::DCP, "dcp", AddressingMode::AbsoluteX, 7, true},

        // 0xE0
        {Instruction::CPX, "cpx", AddressingMode::Immediate, 2, false},
        {Instruction::SBC, "sbc", AddressingMode::IndirectX, 6, false},
        {Instruction::NOP, "nop", AddressingMode::Immediate, 2, true},
        {Instruction::ISC, "isc", AddressingMode::IndirectX, 8, true},
        {Instruction::CPX, "cpx", AddressingMode::ZeroPage, 3, false},
        {Instruction::SBC, "sbc", AddressingMode::ZeroPage, 3, false},
        {Instruction::INC, "inc", AddressingMode::ZeroPage, 5, false},
        {Instruction::ISC, "isc", AddressingMode::ZeroPage, 5, true},
        {Instruction::INX, "inx", AddressingMode::Implied, 2, false},
        {Instruction::SBC, "sbc", AddressingMode::Immediate, 2, false},
        {Instruction::NOP, "nop", AddressingMode::Implied, 2, true},
        {Instruction::SBC, "sbc", AddressingMode::Immediate, 2, true},
        {Instruction::CPX, "cpx", AddressingMode::Absolute, 4, false},
        {Instruction::SBC, "sbc", AddressingMode::Absolute, 4, false},
        {Instruction::INC, "inc", AddressingMode::Absolute, 6, false},
        {Instruction::ISC, "isc", AddressingMode::Absolute, 6, true},

        // 0xF0
        {Instruction::BEQ, "beq", AddressingMode::Relative, 2, false},
        {Instruction::SBC, "sbc", AddressingMode::IndirectY, 5, false},
        {Instruction::KIL, "kil", AddressingMode::Implied, 0, true},
        {Instruction::ISC, "isc", AddressingMode::IndirectY, 8, true},
        {Instruction::NOP, "nop", AddressingMode::ZeroPageX, 4, true},
        {Instruction::SBC, "sbc", AddressingMode::ZeroPageX, 4, false},
        {Instruction::INC, "inc", AddressingMode::ZeroPageX, 6, false},
        {Instruction::ISC, "isc", AddressingMode::ZeroPageX, 6, true},
        {Instruction::SED, "sed", AddressingMode::Implied, 2, false},
        {Instruction::SBC, "sbc", AddressingMode::AbsoluteY, 4, false},
        {Instruction::NOP, "nop", AddressingMode::Implied, 2, true},
        {Instruction::ISC, "isc", AddressingMode::AbsoluteY, 7, true},
        {Instruction::NOP, "nop", AddressingMode::AbsoluteX, 4, true},
        {Instruction::SBC, "sbc", AddressingMode::AbsoluteX, 4, false},
        {Instruction::INC, "inc", AddressingMode::AbsoluteX, 7, false},
        {Instruction::ISC, "isc", AddressingMode::AbsoluteX, 7, true},
} };

CPU6510::CPU6510() {
    reset();
}

void CPU6510::reset() {
    lastWriteToAddr_.resize(65536, 0);
    writeSourceInfo_.resize(65536);

    std::fill(memoryAccess_.begin(), memoryAccess_.end(), 0);

    pc_ = 0;
    sp_ = 0xFD;
    regA_ = regX_ = regY_ = 0;
    statusReg_ = static_cast<u8>(StatusFlag::Interrupt) | static_cast<u8>(StatusFlag::Unused);
    cycles_ = 0;
}

void CPU6510::step() {
    originalPc_ = pc_;

    const u8 opcode = fetchOpcode(pc_);
    pc_++;

    const OpcodeInfo& info = opcodeTable_[opcode];

    execute(info.instruction, info.mode);

    cycles_ += info.cycles;
}

u8 CPU6510::fetchOpcode(u16 addr) {
    memoryAccess_[addr] |= static_cast<u8>(MemoryAccessFlag::Execute);
    memoryAccess_[addr] |= static_cast<u8>(MemoryAccessFlag::OpCode);
    return memory_[addr];
}

u8 CPU6510::fetchOperand(u16 addr) {
    memoryAccess_[addr] |= static_cast<u8>(MemoryAccessFlag::Execute);
    return memory_[addr];
}

u8 CPU6510::readMemory(u16 addr) {
    memoryAccess_[addr] |= static_cast<u8>(MemoryAccessFlag::Read);
    return memory_[addr];
}

u8 CPU6510::readByAddressingMode(u16 addr, AddressingMode mode) {
    switch (mode) {
    case AddressingMode::Indirect:
    case AddressingMode::Immediate:
        return fetchOperand(addr);
    default:
        return readMemory(addr);
    }
}

void CPU6510::writeByte(u16 addr, u8 value) {
    memory_[addr] = value;
}

void CPU6510::writeMemory(u16 addr, u8 value) {
    memoryAccess_[addr] |= static_cast<u8>(MemoryAccessFlag::Write);
    memory_[addr] = value;

    if (onWriteMemoryCallback_) {
        onWriteMemoryCallback_(addr, value);
    }

    if (onCIAWriteCallback_ && (addr >= 0xdc00) && (addr <= 0xdcff)) {
        onCIAWriteCallback_(addr, value);
    }

    lastWriteToAddr_[addr] = originalPc_;
}

void CPU6510::writeBytes(u16 address, std::span<const u8> data) {
    for (size_t i = 0; i < data.size(); ++i) {
        const auto idx = static_cast<u16>(i);
        if (address + idx < memory_.size()) {
            memory_[address + idx] = data[i];
        }
    }
}

void CPU6510::push(u8 value) {
    memory_[0x0100 + sp_] = value;
    sp_--;
}

u8 CPU6510::pop() {
    sp_++;
    return memory_[0x0100 + sp_];
}

u16 CPU6510::readWord(u16 addr) {
    const u8 low = readMemory(addr);
    const u8 high = readMemory(addr + 1);
    return static_cast<u16>(low) | (static_cast<u16>(high) << 8);
}

u16 CPU6510::readWordZeroPage(u8 addr) {
    const u8 low = readMemory(addr);
    const u8 high = readMemory((addr + 1) & 0xFF); // wrap around zero page
    return static_cast<u16>(low) | (static_cast<u16>(high) << 8);
}

void CPU6510::jumpTo(u16 address) {
    setPC(address);
}

void CPU6510::setPC(u16 address) {
    pc_ = address;
}

u16 CPU6510::getPC() const {
    return pc_;
}

void CPU6510::executeFunction(u16 address) {
    // Maximum number of steps to prevent infinite loops
    const int MAX_STEPS = 30000;
    int stepCount = 0;
    bool enableTracing = false; // Set to false to avoid excessive logging

    // Track the last few PCs to detect tight loops
    const int HISTORY_SIZE = 8;
    u16 pcHistory[HISTORY_SIZE] = { 0 };
    int historyIndex = 0;

    // Track potentially dangerous jump targets
    bool jumpToZeroPageTracked = false;
    std::set<u16> reportedProblematicJumps;

    // Simulate JSR manually
    const u16 returnAddress = pc_ - 1; // What JSR would have pushed (the address of the last byte of JSR instruction)

    push((returnAddress >> 8) & 0xFF); // Push high byte
    push(returnAddress & 0xFF);         // Push low byte

    pc_ = address;

    const u8 targetSP = sp_; // After pushing return address (so after manual JSR)
    sidblaster::util::Logger::debug("Executing function at $" + sidblaster::util::wordToHex(address) +
        ", initial SP: $" + sidblaster::util::byteToHex(sp_));

    while (stepCount < MAX_STEPS) {
        const u16 currentPC = pc_;

        // Track PC history for loop detection
        pcHistory[historyIndex] = currentPC;
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;

        // Check for problematic PC values
        if (currentPC < 0x0002) {  // $00 or $01 is definitely a problem
            sidblaster::util::Logger::error("CRITICAL: Execution at $" +
                sidblaster::util::wordToHex(currentPC) +
                " detected - illegal jump target");
            // Break to avoid immediate crash
            break;
        }
        else if (currentPC < 0x0100 && !jumpToZeroPageTracked) {  // Any zero page execution
            sidblaster::util::Logger::warning("Zero page execution detected at $" +
                sidblaster::util::wordToHex(currentPC));
            jumpToZeroPageTracked = true; // Only log once to avoid spam
        }

        // Check for stack issues
        if (sp_ < 0xA0) {  // Potential stack overflow
            sidblaster::util::Logger::warning("Low stack pointer: $" +
                sidblaster::util::byteToHex(sp_) +
                " at PC: $" + sidblaster::util::wordToHex(currentPC));
        }

        // Fetch the opcode
        const u8 opcode = fetchOpcode(pc_);

        // Log the current instruction if tracing is enabled
        if (enableTracing) {
            std::ostringstream logStr;
            logStr << "$" << sidblaster::util::wordToHex(currentPC) << ": "
                << std::string(getMnemonic(opcode)) << " ";

            // Add operand information based on addressing mode
            const auto mode = getAddressingMode(opcode);
            const int size = getInstructionSize(opcode);

            if (size > 1) {
                // Get operand bytes
                if (size == 2) {
                    logStr << "#$" << sidblaster::util::byteToHex(memory_[currentPC + 1]);
                }
                else if (size == 3) {
                    const u16 operand = memory_[currentPC + 1] | (memory_[currentPC + 2] << 8);

                    // Check for problematic jump targets
                    if ((opcode == 0x4C || opcode == 0x20) && // JMP or JSR
                        operand < 0x0100 &&
                        reportedProblematicJumps.find(operand) == reportedProblematicJumps.end()) {

                        sidblaster::util::Logger::warning("Potential problematic jump at $" +
                            sidblaster::util::wordToHex(currentPC) +
                            " to zero page address $" +
                            sidblaster::util::wordToHex(operand));
                        reportedProblematicJumps.insert(operand);
                    }

                    logStr << "$" << sidblaster::util::wordToHex(operand);
                }
            }

            sidblaster::util::Logger::debug(logStr.str());
        }

        // Special handling for key instructions even when tracing is disabled
        const auto mode = getAddressingMode(opcode);
        const int size = getInstructionSize(opcode);

        // Track JMP and JSR instructions to potentially problematic addresses
        if (size == 3 && (opcode == 0x4C || opcode == 0x20)) { // JMP or JSR
            const u16 operand = memory_[currentPC + 1] | (memory_[currentPC + 2] << 8);

            // Check for jumps to very low addresses
            if (operand < 0x0002 &&
                reportedProblematicJumps.find(operand) == reportedProblematicJumps.end()) {

                sidblaster::util::Logger::error("CRITICAL: " + std::string(getMnemonic(opcode)) +
                    " at $" + sidblaster::util::wordToHex(currentPC) +
                    " to illegal address $" +
                    sidblaster::util::wordToHex(operand));
                reportedProblematicJumps.insert(operand);
            }
            else if (operand < 0x0100 &&
                reportedProblematicJumps.find(operand) == reportedProblematicJumps.end()) {

                sidblaster::util::Logger::warning("Suspicious " + std::string(getMnemonic(opcode)) +
                    " at $" + sidblaster::util::wordToHex(currentPC) +
                    " to zero page $" +
                    sidblaster::util::wordToHex(operand));
                reportedProblematicJumps.insert(operand);
            }
        }

        // Track RTS instructions to check return addresses
        if (opcode == 0x60) { // RTS
            if (sp_ < 0xFC) { // Make sure we can safely read from stack
                const u8 lo = memory_[0x0100 + sp_ + 1];
                const u8 hi = memory_[0x0100 + sp_ + 2];
                const u16 returnAddr = (hi << 8) | lo;

                // Log suspicious return addresses
                if (returnAddr < 0x0100) {
                    sidblaster::util::Logger::warning("RTS with suspicious return address: $" +
                        sidblaster::util::wordToHex(returnAddr) +
                        ", SP: $" + sidblaster::util::byteToHex(sp_));
                }

                // Check if this is our function's return
                if (sp_ == targetSP) {
                    sidblaster::util::Logger::debug("Function returning to $" +
                        sidblaster::util::wordToHex(returnAddr + 1) +
                        " after " + std::to_string(stepCount) + " steps");
                }
            }
        }

        // Execute the instruction
        step();
        stepCount++;

        // Check if we've returned from the function
        if (opcode == 0x60) { // RTS
            if (sp_ == targetSP + 2) { // Stack unwound
                sidblaster::util::Logger::debug("Function returned after " + std::to_string(stepCount) + " steps");
                break;
            }
        }
    }

    // Check if we hit the step limit
    if (stepCount >= MAX_STEPS) {
        sidblaster::util::Logger::error("Function execution aborted after " + std::to_string(MAX_STEPS) +
            " steps - possible infinite loop");
        sidblaster::util::Logger::error("Last PC: $" + sidblaster::util::wordToHex(pc_) +
            ", SP: $" + sidblaster::util::byteToHex(sp_));

        // Dump more diagnostic info
        std::ostringstream pcHistoryStr;
        pcHistoryStr << "Recent PC history: ";
        for (int i = 0; i < HISTORY_SIZE; i++) {
            int idx = (historyIndex + i) % HISTORY_SIZE;
            pcHistoryStr << "$" << sidblaster::util::wordToHex(pcHistory[idx]) << " ";
        }
        sidblaster::util::Logger::error(pcHistoryStr.str());
    }
}

void CPU6510::setSP(u8 sp) {
    sp_ = sp;
}

u8 CPU6510::getSP() const {
    return sp_;
}

u16 CPU6510::getAddress(AddressingMode mode) {
    if (mode == AddressingMode::AbsoluteX || mode == AddressingMode::AbsoluteY ||
        mode == AddressingMode::ZeroPageX || mode == AddressingMode::ZeroPageY ||
        mode == AddressingMode::IndirectX || mode == AddressingMode::IndirectY) {

        u8 index = 0;
        if (mode == AddressingMode::AbsoluteY || mode == AddressingMode::ZeroPageY || mode == AddressingMode::IndirectY) {
            index = regY_;
        }
        else if (mode == AddressingMode::AbsoluteX || mode == AddressingMode::ZeroPageX || mode == AddressingMode::IndirectX) {
            index = regX_;
        }
        recordIndexOffset(pc_, index);
    }

    switch (mode) {
    case AddressingMode::Immediate:
        return pc_++;

    case AddressingMode::ZeroPage:
        return fetchOperand(pc_++);

    case AddressingMode::ZeroPageX:
        return (fetchOperand(pc_++) + regX_) & 0xFF;

    case AddressingMode::Absolute: {
        u16 addr = fetchOperand(pc_++);
        addr |= (fetchOperand(pc_++) << 8);
        return addr;
    }

    case AddressingMode::AbsoluteX: {
        const u16 base = fetchOperand(pc_++);
        const u16 highByte = fetchOperand(pc_++);
        const u16 baseAddr = base | (highByte << 8);
        const u16 addr = baseAddr + regX_;

        // Page boundary crossing adds a cycle
        if ((baseAddr & 0xFF00) != (addr & 0xFF00)) {
            cycles_++;
        }
        return addr;
    }

    case AddressingMode::AbsoluteY: {
        const u16 base = fetchOperand(pc_++);
        const u16 highByte = fetchOperand(pc_++);
        const u16 baseAddr = base | (highByte << 8);
        const u16 addr = baseAddr + regY_;

        // Page boundary crossing adds a cycle
        if ((baseAddr & 0xFF00) != (addr & 0xFF00)) {
            cycles_++;
        }
        return addr;
    }

    case AddressingMode::Indirect: {
        const u16 ptr = fetchOperand(pc_++);
        const u16 highByte = fetchOperand(pc_++);
        const u16 indirectAddr = ptr | (highByte << 8);

        // 6502 bug: JMP indirect does not handle page boundaries correctly
        const u8 low = readMemory(indirectAddr);
        const u8 high = readMemory((indirectAddr & 0xFF00) | ((indirectAddr + 1) & 0x00FF));

        return static_cast<u16>(low) | (static_cast<u16>(high) << 8);
    }

    case AddressingMode::IndirectX: {
        const u8 zp = (fetchOperand(pc_++) + regX_) & 0xFF;
        const u16 effectiveAddr = readWordZeroPage(zp);

        // Notify callback if registered
        if (onIndirectReadCallback_) {
            onIndirectReadCallback_(originalPc_, zp, effectiveAddr);
        }

        return effectiveAddr;
    }

    case AddressingMode::IndirectY: {
        const u8 zpAddr = fetchOperand(pc_++);
        const u16 base = readWordZeroPage(zpAddr);
        const u16 addr = base + regY_;

        // Notify callback if registered
        if (onIndirectReadCallback_) {
            onIndirectReadCallback_(originalPc_, zpAddr, addr);
        }

        // Page boundary crossing adds a cycle
        if ((base & 0xFF00) != (addr & 0xFF00)) {
            cycles_++;
        }
        return addr;
    }

    default:
        std::cerr << "Unsupported addressing mode: " << static_cast<int>(mode) << std::endl;
        return 0;
    }
}

// Set Zero and Negative flags based on a value
void CPU6510::setZN(u8 value) {
    setFlag(StatusFlag::Zero, value == 0);
    setFlag(StatusFlag::Negative, (value & 0x80) != 0);
}

void CPU6510::setFlag(StatusFlag flag, bool value) {
    if (value) {
        statusReg_ |= static_cast<u8>(flag);
    }
    else {
        statusReg_ &= ~static_cast<u8>(flag);
    }
}

bool CPU6510::testFlag(StatusFlag flag) const {
    return (statusReg_ & static_cast<u8>(flag)) != 0;
}

// Instruction execution entry point
void CPU6510::execute(Instruction instr, AddressingMode mode) {
    // Dispatch to appropriate instruction handler
    executeInstruction(instr, mode);
}

// Execute an instruction based on its type
void CPU6510::executeInstruction(Instruction instr, AddressingMode mode) {
    // Group instructions by function
    switch (instr) {
        // Load instructions
    case Instruction::LDA:
    case Instruction::LDX:
    case Instruction::LDY:
    case Instruction::LAX:
        executeLoad(instr, mode);
        break;

        // Store instructions
    case Instruction::STA:
    case Instruction::STX:
    case Instruction::STY:
    case Instruction::SAX:
        executeStore(instr, mode);
        break;

        // Arithmetic instructions
    case Instruction::ADC:
    case Instruction::SBC:
    case Instruction::INC:
    case Instruction::INX:
    case Instruction::INY:
    case Instruction::DEC:
    case Instruction::DEX:
    case Instruction::DEY:
        executeArithmetic(instr, mode);
        break;

        // Logical instructions
    case Instruction::AND:
    case Instruction::ORA:
    case Instruction::EOR:
    case Instruction::BIT:
        executeLogical(instr, mode);
        break;

        // Branch instructions
    case Instruction::BCC:
    case Instruction::BCS:
    case Instruction::BEQ:
    case Instruction::BMI:
    case Instruction::BNE:
    case Instruction::BPL:
    case Instruction::BVC:
    case Instruction::BVS:
        executeBranch(instr, mode);
        break;

        // Jump instructions
    case Instruction::JMP:
    case Instruction::JSR:
    case Instruction::RTS:
    case Instruction::RTI:
    case Instruction::BRK:
        executeJump(instr, mode);
        break;

        // Stack instructions
    case Instruction::PHA:
    case Instruction::PHP:
    case Instruction::PLA:
    case Instruction::PLP:
        executeStack(instr, mode);
        break;

        // Register transfer instructions
    case Instruction::TAX:
    case Instruction::TAY:
    case Instruction::TXA:
    case Instruction::TYA:
    case Instruction::TSX:
    case Instruction::TXS:
        executeRegister(instr, mode);
        break;

        // Flag instructions
    case Instruction::CLC:
    case Instruction::CLD:
    case Instruction::CLI:
    case Instruction::CLV:
    case Instruction::SEC:
    case Instruction::SED:
    case Instruction::SEI:
        executeFlag(instr, mode);
        break;

        // Shift instructions
    case Instruction::ASL:
    case Instruction::LSR:
    case Instruction::ROL:
    case Instruction::ROR:
        executeShift(instr, mode);
        break;

        // Compare instructions
    case Instruction::CMP:
    case Instruction::CPX:
    case Instruction::CPY:
        executeCompare(instr, mode);
        break;

        // NOP instruction
    case Instruction::NOP:
        // No operation, just consume cycles
        break;

        // Illegal instructions
    case Instruction::SLO:
    case Instruction::RLA:
    case Instruction::SRE:
    case Instruction::RRA:
    case Instruction::DCP:
    case Instruction::ISC:
    case Instruction::ANC:
    case Instruction::ALR:
    case Instruction::ARR:
    case Instruction::AXS:
    case Instruction::KIL:
    case Instruction::LAS:
    case Instruction::AHX:
    case Instruction::TAS:
    case Instruction::SHA:
    case Instruction::SHX:
    case Instruction::SHY:
    case Instruction::XAA:
        executeIllegal(instr, mode);
        break;

    default:
        std::cerr << "Unimplemented instruction: " << static_cast<int>(instr) << std::endl;
        break;
    }
}

// Load instruction implementation (LDA, LDX, LDY, LAX)
void CPU6510::executeLoad(Instruction instr, AddressingMode mode) {
    const u16 addr = getAddress(mode);
    const u8 value = readByAddressingMode(addr, mode);

    u8 index = 0;
    if (mode == AddressingMode::AbsoluteY || mode == AddressingMode::ZeroPageY || mode == AddressingMode::IndirectY) {
        index = regY_;
    }
    else if (mode == AddressingMode::AbsoluteX || mode == AddressingMode::ZeroPageX || mode == AddressingMode::IndirectX) {
        index = regX_;
    }

    // Update appropriate register based on instruction
    switch (instr) {
    case Instruction::LDA:
        regA_ = value;
        regSourceA_ = {
            RegisterSourceInfo::SourceType::Memory,
            addr,
            value,
            index
        };
        break;

    case Instruction::LDX:
        regX_ = value;
        regSourceX_ = {
            RegisterSourceInfo::SourceType::Memory,
            addr,
            value,
            0
        };
        break;

    case Instruction::LDY:
        regY_ = value;
        regSourceY_ = {
            RegisterSourceInfo::SourceType::Memory,
            addr,
            value,
            0
        };
        break;

    case Instruction::LAX:
        // LAX = LDA + LDX combined (illegal opcode)
        regA_ = regX_ = value;
        regSourceA_ = regSourceX_ = {
            RegisterSourceInfo::SourceType::Memory,
            addr,
            value,
            0
        };
        break;

    default:
        break;
    }

    // Set appropriate flags
    setZN(instr == Instruction::LDX || instr == Instruction::LAX ? regX_ :
        instr == Instruction::LDY ? regY_ : regA_);
}

// Store instruction implementation (STA, STX, STY, SAX)
void CPU6510::executeStore(Instruction instr, AddressingMode mode) {
    const u16 addr = getAddress(mode);

    switch (instr) {
    case Instruction::STA:
        writeMemory(addr, regA_);
        writeSourceInfo_[addr] = getRegSourceA();
        break;

    case Instruction::STX:
        writeMemory(addr, regX_);
        writeSourceInfo_[addr] = getRegSourceX();
        break;

    case Instruction::STY:
        writeMemory(addr, regY_);
        writeSourceInfo_[addr] = getRegSourceY();
        break;

    case Instruction::SAX:
        // SAX = Store A AND X (illegal opcode)
        writeMemory(addr, regA_ & regX_);
        break;

    default:
        break;
    }
}

// Arithmetic instruction implementation
void CPU6510::executeArithmetic(Instruction instr, AddressingMode mode) {
    switch (instr) {
    case Instruction::ADC: {
        const u16 addr = getAddress(mode);
        const u8 value = readByAddressingMode(addr, mode);

        if (testFlag(StatusFlag::Decimal)) {
            // BCD arithmetic in decimal mode
            u8 al = (regA_ & 0x0F) + (value & 0x0F) + (testFlag(StatusFlag::Carry) ? 1 : 0);
            u8 ah = (regA_ >> 4) + (value >> 4);

            if (al > 9) {
                al -= 10;
                ah++;
            }

            if (ah > 9) {
                ah -= 10;
                setFlag(StatusFlag::Carry, true);
            }
            else {
                setFlag(StatusFlag::Carry, false);
            }

            regA_ = (ah << 4) | (al & 0x0F);
            setZN(regA_);
            // Note: Overflow in decimal mode is undefined on the 6502
        }
        else {
            // Normal binary arithmetic
            const u16 sum = static_cast<u16>(regA_) + static_cast<u16>(value) + (testFlag(StatusFlag::Carry) ? 1 : 0);

            setFlag(StatusFlag::Carry, sum > 0xFF);
            setFlag(StatusFlag::Zero, (sum & 0xFF) == 0);
            setFlag(StatusFlag::Overflow, ((~(regA_ ^ value) & (regA_ ^ sum) & 0x80) != 0));
            setFlag(StatusFlag::Negative, (sum & 0x80) != 0);

            regA_ = static_cast<u8>(sum & 0xFF);
        }
        break;
    }

    case Instruction::SBC: {
        const u16 addr = getAddress(mode);
        const u8 value = readByAddressingMode(addr, mode);

        // SBC is ADC with inverted operand
        const u8 invertedValue = value ^ 0xFF;

        if (testFlag(StatusFlag::Decimal)) {
            // BCD arithmetic in decimal mode
            u8 al = (regA_ & 0x0F) + (invertedValue & 0x0F) + (testFlag(StatusFlag::Carry) ? 1 : 0);
            u8 ah = (regA_ >> 4) + (invertedValue >> 4);

            if (al > 9) {
                al -= 10;
                ah++;
            }

            if (ah > 9) {
                ah -= 10;
                setFlag(StatusFlag::Carry, true);
            }
            else {
                setFlag(StatusFlag::Carry, false);
            }

            regA_ = (ah << 4) | (al & 0x0F);
            setZN(regA_);
        }
        else {
            // Normal binary arithmetic
            const u16 diff = static_cast<u16>(regA_) + static_cast<u16>(invertedValue) + (testFlag(StatusFlag::Carry) ? 1 : 0);

            setFlag(StatusFlag::Carry, diff > 0xFF);
            setFlag(StatusFlag::Zero, (diff & 0xFF) == 0);
            setFlag(StatusFlag::Overflow, ((~(regA_ ^ invertedValue) & (regA_ ^ diff) & 0x80) != 0));
            setFlag(StatusFlag::Negative, (diff & 0x80) != 0);

            regA_ = static_cast<u8>(diff & 0xFF);
        }
        break;
    }

    case Instruction::INC: {
        const u16 addr = getAddress(mode);
        u8 value = readByAddressingMode(addr, mode);
        value++;
        writeMemory(addr, value);
        setZN(value);
        break;
    }

    case Instruction::INX:
        regX_++;
        setZN(regX_);
        break;

    case Instruction::INY:
        regY_++;
        setZN(regY_);
        break;

    case Instruction::DEC: {
        const u16 addr = getAddress(mode);
        u8 value = readByAddressingMode(addr, mode);
        value--;
        writeMemory(addr, value);
        setZN(value);
        break;
    }

    case Instruction::DEX:
        regX_--;
        setZN(regX_);
        break;

    case Instruction::DEY:
        regY_--;
        setZN(regY_);
        break;

    default:
        break;
    }
}

// Logical instruction implementation
void CPU6510::executeLogical(Instruction instr, AddressingMode mode) {
    const u16 addr = getAddress(mode);
    const u8 value = readByAddressingMode(addr, mode);

    switch (instr) {
    case Instruction::AND:
        regA_ &= value;
        setZN(regA_);
        break;

    case Instruction::ORA:
        regA_ |= value;
        setZN(regA_);
        break;

    case Instruction::EOR:
        regA_ ^= value;
        setZN(regA_);
        break;

    case Instruction::BIT:
        // Set zero flag based on AND result, but don't change accumulator
        setFlag(StatusFlag::Zero, (regA_ & value) == 0);
        // BIT also sets negative and overflow flags directly from memory byte
        setFlag(StatusFlag::Negative, (value & 0x80) != 0);
        setFlag(StatusFlag::Overflow, (value & 0x40) != 0);
        break;

    default:
        break;
    }
}

// Branch instruction implementation
void CPU6510::executeBranch(Instruction instr, AddressingMode mode) {
    const i8 offset = static_cast<i8>(fetchOperand(pc_++));
    bool branchTaken = false;

    // Determine if branch should be taken based on instruction and flags
    switch (instr) {
    case Instruction::BCC:
        branchTaken = !testFlag(StatusFlag::Carry);
        break;

    case Instruction::BCS:
        branchTaken = testFlag(StatusFlag::Carry);
        break;

    case Instruction::BEQ:
        branchTaken = testFlag(StatusFlag::Zero);
        break;

    case Instruction::BMI:
        branchTaken = testFlag(StatusFlag::Negative);
        break;

    case Instruction::BNE:
        branchTaken = !testFlag(StatusFlag::Zero);
        break;

    case Instruction::BPL:
        branchTaken = !testFlag(StatusFlag::Negative);
        break;

    case Instruction::BVC:
        branchTaken = !testFlag(StatusFlag::Overflow);
        break;

    case Instruction::BVS:
        branchTaken = testFlag(StatusFlag::Overflow);
        break;

    default:
        break;
    }

    // If branch is taken, update PC and add cycles
    if (branchTaken) {
        const u16 oldPC = pc_;
        pc_ += offset;
        memoryAccess_[pc_] |= static_cast<u8>(MemoryAccessFlag::JumpTarget);

        // Branch taken: add 1 cycle
        cycles_++;

        // Page boundary crossing: add another cycle
        if ((oldPC & 0xFF00) != (pc_ & 0xFF00)) {
            cycles_++;
        }
    }
}

// Jump instruction implementation
void CPU6510::executeJump(Instruction instr, AddressingMode mode) {
    switch (instr) {
    case Instruction::JMP: {
        const u16 addr = getAddress(mode);
        memoryAccess_[addr] |= static_cast<u8>(MemoryAccessFlag::JumpTarget);
        pc_ = addr;
        break;
    }

    case Instruction::JSR: {
        const u16 addr = getAddress(mode);
        memoryAccess_[addr] |= static_cast<u8>(MemoryAccessFlag::JumpTarget);
        pc_--; // Push address of last byte of JSR
        push((pc_ >> 8) & 0xFF); // Push high byte
        push(pc_ & 0xFF);        // Push low byte
        pc_ = addr;
        break;
    }

    case Instruction::RTS: {
        const u8 lo = pop();
        const u8 hi = pop();
        pc_ = (hi << 8) | lo;
        pc_++; // RTS increments PC after popping
        break;
    }

    case Instruction::RTI: {
        statusReg_ = pop();
        const u8 lo = pop();
        const u8 hi = pop();
        pc_ = (hi << 8) | lo;
        break;
    }

    case Instruction::BRK: {
        pc_++; // Skip signature byte
        push((pc_ >> 8) & 0xFF); // Push high byte of PC
        push(pc_ & 0xFF);        // Push low byte of PC
        push(statusReg_ | static_cast<u8>(StatusFlag::Break) | static_cast<u8>(StatusFlag::Unused));
        setFlag(StatusFlag::Interrupt, true);
        pc_ = readMemory(0xFFFE) | (readMemory(0xFFFF) << 8);
        break;
    }

    default:
        break;
    }
}

// Stack instruction implementation
void CPU6510::executeStack(Instruction instr, AddressingMode mode) {
    switch (instr) {
    case Instruction::PHA:
        push(regA_);
        break;

    case Instruction::PHP:
        push(statusReg_ | static_cast<u8>(StatusFlag::Break) | static_cast<u8>(StatusFlag::Unused));
        break;

    case Instruction::PLA:
        regA_ = pop();
        setZN(regA_);
        break;

    case Instruction::PLP:
        statusReg_ = pop();
        break;

    default:
        break;
    }
}

// Register transfer instruction implementation
void CPU6510::executeRegister(Instruction instr, AddressingMode mode) {
    switch (instr) {
    case Instruction::TAX:
        regX_ = regA_;
        setZN(regX_);
        break;

    case Instruction::TAY:
        regY_ = regA_;
        setZN(regY_);
        break;

    case Instruction::TXA:
        regA_ = regX_;
        setZN(regA_);
        break;

    case Instruction::TYA:
        regA_ = regY_;
        setZN(regA_);
        break;

    case Instruction::TSX:
        regX_ = sp_;
        setZN(regX_);
        break;

    case Instruction::TXS:
        sp_ = regX_;
        break;

    default:
        break;
    }
}

// Flag instruction implementation
void CPU6510::executeFlag(Instruction instr, AddressingMode mode) {
    switch (instr) {
    case Instruction::CLC:
        setFlag(StatusFlag::Carry, false);
        break;

    case Instruction::CLD:
        setFlag(StatusFlag::Decimal, false);
        break;

    case Instruction::CLI:
        setFlag(StatusFlag::Interrupt, false);
        break;

    case Instruction::CLV:
        setFlag(StatusFlag::Overflow, false);
        break;

    case Instruction::SEC:
        setFlag(StatusFlag::Carry, true);
        break;

    case Instruction::SED:
        setFlag(StatusFlag::Decimal, true);
        break;

    case Instruction::SEI:
        setFlag(StatusFlag::Interrupt, true);
        break;

    default:
        break;
    }
}

// Shift instruction implementation
void CPU6510::executeShift(Instruction instr, AddressingMode mode) {
    u16 addr = 0;
    u8 value = 0;

    // Determine operand source
    if (mode == AddressingMode::Accumulator) {
        value = regA_;
    }
    else {
        addr = getAddress(mode);
        value = readByAddressingMode(addr, mode);
    }

    switch (instr) {
    case Instruction::ASL: {
        // Arithmetic Shift Left
        setFlag(StatusFlag::Carry, (value & 0x80) != 0);
        value <<= 1;
        break;
    }

    case Instruction::LSR: {
        // Logical Shift Right
        setFlag(StatusFlag::Carry, (value & 0x01) != 0);
        value >>= 1;
        break;
    }

    case Instruction::ROL: {
        // Rotate Left
        const bool oldCarry = testFlag(StatusFlag::Carry);
        setFlag(StatusFlag::Carry, (value & 0x80) != 0);
        value = (value << 1) | (oldCarry ? 1 : 0);
        break;
    }

    case Instruction::ROR: {
        // Rotate Right
        const bool oldCarry = testFlag(StatusFlag::Carry);
        setFlag(StatusFlag::Carry, (value & 0x01) != 0);
        value = (value >> 1) | (oldCarry ? 0x80 : 0x00);
        break;
    }

    default:
        break;
    }

    // Update flags and store result
    setZN(value);

    if (mode == AddressingMode::Accumulator) {
        regA_ = value;
    }
    else {
        writeMemory(addr, value);
    }
}

// Compare instruction implementation
void CPU6510::executeCompare(Instruction instr, AddressingMode mode) {
    const u16 addr = getAddress(mode);
    const u8 value = readByAddressingMode(addr, mode);
    u8 regValue = 0;

    // Select register to compare based on instruction
    switch (instr) {
    case Instruction::CMP:
        regValue = regA_;
        break;

    case Instruction::CPX:
        regValue = regX_;
        break;

    case Instruction::CPY:
        regValue = regY_;
        break;

    default:
        break;
    }

    // Perform comparison and set flags
    setFlag(StatusFlag::Carry, regValue >= value);
    setFlag(StatusFlag::Zero, regValue == value);
    setFlag(StatusFlag::Negative, ((regValue - value) & 0x80) != 0);
}

// Implementation for illegal/undocumented instructions
void CPU6510::executeIllegal(Instruction instr, AddressingMode mode) {
    // Note: These implementations are based on documented behavior of illegal opcodes

    switch (instr) {
    case Instruction::SLO: {
        // SLO = ASL + ORA
        const u16 addr = getAddress(mode);
        u8 value = readByAddressingMode(addr, mode);

        // ASL part
        setFlag(StatusFlag::Carry, (value & 0x80) != 0);
        value <<= 1;
        writeMemory(addr, value);

        // ORA part
        regA_ |= value;
        setZN(regA_);
        break;
    }

    case Instruction::RLA: {
        // RLA = ROL + AND
        const u16 addr = getAddress(mode);
        u8 value = readByAddressingMode(addr, mode);

        // ROL part
        const bool oldCarry = testFlag(StatusFlag::Carry);
        setFlag(StatusFlag::Carry, (value & 0x80) != 0);
        value = (value << 1) | (oldCarry ? 1 : 0);
        writeMemory(addr, value);

        // AND part
        regA_ &= value;
        setZN(regA_);
        break;
    }

    case Instruction::SRE: {
        // SRE = LSR + EOR
        const u16 addr = getAddress(mode);
        u8 value = readByAddressingMode(addr, mode);

        // LSR part
        setFlag(StatusFlag::Carry, (value & 0x01) != 0);
        value >>= 1;
        writeMemory(addr, value);

        // EOR part
        regA_ ^= value;
        setZN(regA_);
        break;
    }

    case Instruction::RRA: {
        // RRA = ROR + ADC
        const u16 addr = getAddress(mode);
        u8 value = readByAddressingMode(addr, mode);

        // ROR part
        const bool oldCarry = testFlag(StatusFlag::Carry);
        setFlag(StatusFlag::Carry, (value & 0x01) != 0);
        value = (value >> 1) | (oldCarry ? 0x80 : 0x00);
        writeMemory(addr, value);

        // ADC part (simplified, not handling decimal mode here)
        const u16 sum = static_cast<u16>(regA_) + static_cast<u16>(value) + (testFlag(StatusFlag::Carry) ? 1 : 0);

        setFlag(StatusFlag::Carry, sum > 0xFF);
        setFlag(StatusFlag::Zero, (sum & 0xFF) == 0);
        setFlag(StatusFlag::Overflow, ((~(regA_ ^ value) & (regA_ ^ sum) & 0x80) != 0));
        setFlag(StatusFlag::Negative, (sum & 0x80) != 0);

        regA_ = static_cast<u8>(sum & 0xFF);
        break;
    }

    case Instruction::DCP: {
        // DCP = DEC + CMP
        const u16 addr = getAddress(mode);
        u8 value = readByAddressingMode(addr, mode);

        // DEC part
        value--;
        writeMemory(addr, value);

        // CMP part
        setFlag(StatusFlag::Carry, regA_ >= value);
        setFlag(StatusFlag::Zero, regA_ == value);
        setFlag(StatusFlag::Negative, ((regA_ - value) & 0x80) != 0);
        break;
    }

    case Instruction::ISC: {
        // ISC = INC + SBC
        const u16 addr = getAddress(mode);
        u8 value = readByAddressingMode(addr, mode);

        // INC part
        value++;
        writeMemory(addr, value);

        // SBC part (using inverted operand)
        const u8 invertedValue = value ^ 0xFF;
        const u16 diff = static_cast<u16>(regA_) + static_cast<u16>(invertedValue) + (testFlag(StatusFlag::Carry) ? 1 : 0);

        setFlag(StatusFlag::Carry, diff > 0xFF);
        setFlag(StatusFlag::Zero, (diff & 0xFF) == 0);
        setFlag(StatusFlag::Overflow, ((~(regA_ ^ invertedValue) & (regA_ ^ diff) & 0x80) != 0));
        setFlag(StatusFlag::Negative, (diff & 0x80) != 0);

        regA_ = static_cast<u8>(diff & 0xFF);
        break;
    }

    case Instruction::ANC: {
        // ANC = AND + set carry from bit 7
        const u16 addr = getAddress(mode);
        const u8 value = readByAddressingMode(addr, mode);

        regA_ &= value;
        setZN(regA_);
        setFlag(StatusFlag::Carry, (regA_ & 0x80) != 0);
        break;
    }

    case Instruction::ALR: {
        // ALR = AND + LSR
        const u16 addr = getAddress(mode);
        const u8 value = readByAddressingMode(addr, mode);

        // AND part
        regA_ &= value;

        // LSR part
        setFlag(StatusFlag::Carry, (regA_ & 0x01) != 0);
        regA_ >>= 1;
        setZN(regA_);
        break;
    }

    case Instruction::ARR: {
        // ARR = AND + ROR (with special flag behavior)
        const u16 addr = getAddress(mode);
        const u8 value = readByAddressingMode(addr, mode);

        // AND part
        regA_ &= value;

        // ROR part
        const bool oldCarry = testFlag(StatusFlag::Carry);
        regA_ = (regA_ >> 1) | (oldCarry ? 0x80 : 0x00);

        // Special flag behavior
        setFlag(StatusFlag::Zero, regA_ == 0);
        setFlag(StatusFlag::Negative, (regA_ & 0x80) != 0);
        setFlag(StatusFlag::Carry, (regA_ & 0x40) != 0);
        setFlag(StatusFlag::Overflow, ((regA_ & 0x40) ^ ((regA_ & 0x20) << 1)) != 0);
        break;
    }

    case Instruction::AXS: {
        // AXS = A AND X, then subtract without borrow
        const u16 addr = getAddress(mode);
        const u8 value = readByAddressingMode(addr, mode);

        const u8 temp = regA_ & regX_;
        const u16 result = static_cast<u16>(temp) - static_cast<u16>(value);

        setFlag(StatusFlag::Carry, temp >= value);
        setFlag(StatusFlag::Zero, (result & 0xFF) == 0);
        setFlag(StatusFlag::Negative, (result & 0x80) != 0);

        regX_ = static_cast<u8>(result & 0xFF);
        break;
    }

    case Instruction::LAS: {
        // LAS = Memory AND SP -> A, X, SP
        const u16 addr = getAddress(mode);
        const u8 value = readByAddressingMode(addr, mode);

        const u8 result = value & sp_;
        regA_ = regX_ = sp_ = result;
        setZN(result);
        break;
    }

    case Instruction::KIL: {
        // KIL = Processor lock-up
        // In real hardware, this would cause the CPU to stop
        // For emulation, we just do nothing
        pc_--; // Stay on this instruction
        break;
    }

                         // These remaining illegal instructions have more complex or unstable behavior
                         // Simplified implementations provided for emulation purposes

    case Instruction::XAA: {
        // XAA = Behavior varies by processor and conditions
        const u16 addr = getAddress(mode);
        const u8 value = readByAddressingMode(addr, mode);

        // Simplified model: X -> A, then AND with value
        regA_ = regX_ & value;
        setZN(regA_);
        break;
    }

    case Instruction::AHX:
    case Instruction::SHA: {
        // AHX/SHA = Store A AND X AND (high byte + 1)
        const u16 addr = getAddress(mode);
        const u8 high = (addr >> 8) & 0xFF;
        const u8 result = regA_ & regX_ & (high + 1);

        writeMemory(addr, result);
        break;
    }

    case Instruction::SHX: {
        // SHX = Store X AND (high byte + 1)
        const u16 addr = getAddress(mode);
        const u8 high = (addr >> 8) & 0xFF;
        const u8 result = regX_ & (high + 1);

        writeMemory(addr, result);
        break;
    }

    case Instruction::SHY: {
        // SHY = Store Y AND (high byte + 1)
        const u16 addr = getAddress(mode);
        const u8 high = (addr >> 8) & 0xFF;
        const u8 result = regY_ & (high + 1);

        writeMemory(addr, result);
        break;
    }

    case Instruction::TAS: {
        // TAS = A AND X -> SP, then store SP AND (high byte + 1)
        const u16 addr = getAddress(mode);
        const u8 high = (addr >> 8) & 0xFF;

        sp_ = regA_ & regX_;
        const u8 result = sp_ & (high + 1);

        writeMemory(addr, result);
        break;
    }

    default:
        break;
    }
}

// Utility functions

void CPU6510::loadData(const std::string& filename, u16 loadAddress) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    u8 byte;
    while (file.read(reinterpret_cast<char*>(&byte), 1)) {
        if (loadAddress < memory_.size()) {
            memory_[loadAddress++] = byte;
        }
        else {
            throw std::runtime_error("Attempted to load data beyond memory bounds");
        }
    }
}

u64 CPU6510::getCycles() const {
    return cycles_;
}

void CPU6510::setCycles(u64 newCycles) {
    cycles_ = newCycles;
}

void CPU6510::resetCycles() {
    cycles_ = 0;
}

void CPU6510::dumpMemoryAccess(const std::string& filename) {
    std::ofstream file(filename);
    if (!file) {
        return;
    }

    for (u32 addr = 0; addr < 65536; ++addr) {
        if (memoryAccess_[addr] != 0) {
            file << std::hex << std::setw(4) << std::setfill('0') << addr << ": ";

            file << ((memoryAccess_[addr] & static_cast<u8>(MemoryAccessFlag::Execute)) ? "E" : ".");
            file << ((memoryAccess_[addr] & static_cast<u8>(MemoryAccessFlag::OpCode)) ? "1" : ".");
            file << ((memoryAccess_[addr] & static_cast<u8>(MemoryAccessFlag::Read)) ? "R" : ".");
            file << ((memoryAccess_[addr] & static_cast<u8>(MemoryAccessFlag::Write)) ? "W" : ".");
            file << ((memoryAccess_[addr] & static_cast<u8>(MemoryAccessFlag::JumpTarget)) ? "J" : ".");

            file << "\n";
        }
    }
}

std::string_view CPU6510::getMnemonic(u8 opcode) const {
    return opcodeTable_[opcode].mnemonic;
}

u8 CPU6510::getInstructionSize(u8 opcode) const {
    const AddressingMode mode = opcodeTable_[opcode].mode;
    switch (mode) {
    case AddressingMode::Immediate:
    case AddressingMode::ZeroPage:
    case AddressingMode::ZeroPageX:
    case AddressingMode::ZeroPageY:
    case AddressingMode::Relative:
    case AddressingMode::IndirectX:
    case AddressingMode::IndirectY:
        return 2;
    case AddressingMode::Absolute:
    case AddressingMode::AbsoluteX:
    case AddressingMode::AbsoluteY:
    case AddressingMode::Indirect:
        return 3;
    case AddressingMode::Accumulator:
    case AddressingMode::Implied:
    default:
        return 1;
    }
}

AddressingMode CPU6510::getAddressingMode(u8 opcode) const {
    return opcodeTable_[opcode].mode;
}

bool CPU6510::isIllegalInstruction(u8 opcode) const {
    return opcodeTable_[opcode].illegal;
}

void CPU6510::copyMemoryBlock(u16 start, std::span<const u8> data) {
    for (size_t i = 0; i < data.size(); ++i) {
        const auto idx = static_cast<u16>(i);
        if (start + idx < memory_.size()) {
            memory_[start + idx] = data[i];
        }
    }
}

void CPU6510::recordIndexOffset(u16 pc, u8 offset) {
    pcIndexRanges_[pc].update(offset);
}

std::pair<u8, u8> CPU6510::getIndexRange(u16 pc) const {
    auto it = pcIndexRanges_.find(pc);
    if (it == pcIndexRanges_.end()) {
        return { 0, 0 };
    }
    return it->second.getRange();
}

// Accessor methods
const std::vector<u16>& CPU6510::getLastWriteToAddr() const {
    return lastWriteToAddr_;
}

u16 CPU6510::getLastWriteTo(u16 addr) const {
    return lastWriteToAddr_[addr];
}

RegisterSourceInfo CPU6510::getRegSourceA() const {
    return regSourceA_;
}

RegisterSourceInfo CPU6510::getRegSourceX() const {
    return regSourceX_;
}

RegisterSourceInfo CPU6510::getRegSourceY() const {
    return regSourceY_;
}

RegisterSourceInfo CPU6510::getWriteSourceInfo(u16 addr) const {
    return writeSourceInfo_[addr];
}

// Callback methods
void CPU6510::setOnIndirectReadCallback(IndirectReadCallback callback) {
    onIndirectReadCallback_ = std::move(callback);
}

void CPU6510::setOnWriteMemoryCallback(MemoryWriteCallback callback) {
    onWriteMemoryCallback_ = std::move(callback);
}

void CPU6510::setOnCIAWriteCallback(MemoryWriteCallback callback) {
    onCIAWriteCallback_ = std::move(callback);
}