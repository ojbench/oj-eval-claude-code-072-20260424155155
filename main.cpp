#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

// RISC-V registers
enum Reg {
    zero = 0, ra, sp, gp, tp, t0, t1, t2,
    s0, s1, a0, a1, a2, a3, a4, a5,
    a6, a7, s2, s3, s4, s5, s6, s7,
    s8, s9, s10, s11, t3, t4, t5, t6
};

class RISC_V_Simulator {
private:
    // 32 registers, each 32 bits
    int32_t regs[32] = {0};

    // Memory (4KB for simplicity)
    uint8_t memory[4096] = {0};

    // Program counter
    uint32_t pc = 0;

    // Cycle counter
    uint64_t cycles = 0;

    // Instruction count
    uint64_t instructions = 0;

public:
    RISC_V_Simulator() {
        // Initialize stack pointer to end of memory
        regs[sp] = 4096;
    }

    void reset() {
        for (int i = 0; i < 32; i++) {
            regs[i] = 0;
        }
        regs[sp] = 4096;
        pc = 0;
        cycles = 0;
        instructions = 0;
    }

    // Load program into memory
    void load_program(const vector<uint32_t>& program) {
        for (size_t i = 0; i < program.size() && i * 4 < 4096; i++) {
            uint32_t instr = program[i];
            memory[i*4] = instr & 0xFF;
            memory[i*4 + 1] = (instr >> 8) & 0xFF;
            memory[i*4 + 2] = (instr >> 16) & 0xFF;
            memory[i*4 + 3] = (instr >> 24) & 0xFF;
        }
    }

    // Fetch instruction from memory
    uint32_t fetch() {
        if (pc + 3 >= 4096) return 0;

        uint32_t instr = 0;
        instr |= memory[pc];
        instr |= memory[pc + 1] << 8;
        instr |= memory[pc + 2] << 16;
        instr |= memory[pc + 3] << 24;

        pc += 4;
        return instr;
    }

    // Decode and execute instruction
    bool execute(uint32_t instr) {
        if (instr == 0) return false; // NOP or end

        uint32_t opcode = instr & 0x7F;
        uint32_t rd = (instr >> 7) & 0x1F;
        uint32_t funct3 = (instr >> 12) & 0x7;
        uint32_t rs1 = (instr >> 15) & 0x1F;
        uint32_t rs2 = (instr >> 20) & 0x1F;
        uint32_t funct7 = (instr >> 25) & 0x7F;

        // Basic instruction implementation
        switch (opcode) {
            case 0x13: { // ADDI
                int32_t imm = (int32_t)(instr & 0xFFF00000) >> 20;
                if (imm & 0x800) imm |= 0xFFFFF000;
                regs[rd] = regs[rs1] + imm;
                break;
            }
            case 0x33: { // ADD, SUB, etc.
                switch (funct3) {
                    case 0x0: // ADD or SUB
                        if (funct7 == 0x00) {
                            regs[rd] = regs[rs1] + regs[rs2];
                        } else if (funct7 == 0x20) {
                            regs[rd] = regs[rs1] - regs[rs2];
                        }
                        break;
                    // Add more instructions as needed
                }
                break;
            }
            case 0x63: { // BEQ, BNE, etc.
                int32_t imm = ((instr >> 31) & 1) << 12 |
                             ((instr >> 25) & 0x3F) << 5 |
                             ((instr >> 8) & 0xF) << 1 |
                             ((instr >> 7) & 1) << 11;
                if (imm & 0x1000) imm |= 0xFFFFE000;

                bool branch_taken = false;
                switch (funct3) {
                    case 0x0: // BEQ
                        branch_taken = (regs[rs1] == regs[rs2]);
                        break;
                    case 0x1: // BNE
                        branch_taken = (regs[rs1] != regs[rs2]);
                        break;
                }

                if (branch_taken) {
                    pc = pc - 4 + imm; // Adjust for pc already incremented in fetch
                }
                break;
            }
            case 0x6F: { // JAL
                int32_t imm = ((instr >> 31) & 1) << 20 |
                             ((instr >> 21) & 0x3FF) << 1 |
                             ((instr >> 20) & 1) << 11 |
                             ((instr >> 12) & 0xFF) << 12;
                if (imm & 0x100000) imm |= 0xFFE00000;

                regs[rd] = pc; // Return address
                pc = pc - 4 + imm; // Adjust for pc already incremented
                break;
            }
            // Add more instruction types as needed
        }

        // x0 is always zero
        if (rd == 0) regs[0] = 0;

        instructions++;
        cycles++;

        return true;
    }

    void run() {
        while (true) {
            uint32_t instr = fetch();
            if (!execute(instr)) {
                break;
            }
        }
    }

    // Get register value
    int32_t get_reg(int reg_num) {
        if (reg_num < 0 || reg_num >= 32) return 0;
        return regs[reg_num];
    }

    // Get cycle count
    uint64_t get_cycles() { return cycles; }

    // Get instruction count
    uint64_t get_instructions() { return instructions; }
};

int main() {
    RISC_V_Simulator simulator;

    // Read input from stdin
    vector<uint32_t> program;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        // Try to parse as hex or decimal
        uint32_t instr;
        stringstream ss(line);

        if (line.find("0x") == 0 || line.find("0X") == 0) {
            ss >> hex >> instr;
        } else {
            ss >> dec >> instr;
        }

        if (!ss.fail()) {
            program.push_back(instr);
        }
    }

    if (program.empty()) {
        // For testing, use a simple program
        program = {
            0x00000093, // ADDI x1, x0, 0  (li x1, 0)
            0x00108093, // ADDI x1, x1, 1  (addi x1, x1, 1)
            0x00108093, // ADDI x1, x1, 1  (addi x1, x1, 1)
            0x00108093, // ADDI x1, x1, 1  (addi x1, x1, 1)
            0x00000073  // ECALL (simulate end)
        };
    }

    simulator.load_program(program);
    simulator.run();

    // Output results (format depends on problem requirements)
    // For now, output register values
    cout << "Register values:" << endl;
    for (int i = 0; i < 32; i++) {
        cout << "x" << i << ": " << simulator.get_reg(i) << endl;
    }

    cout << "Cycles: " << simulator.get_cycles() << endl;
    cout << "Instructions: " << simulator.get_instructions() << endl;

    return 0;
}