#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#define REG_NUM 32
#define DATA_MEM_SIZE 32
#define INS_MEM_SIZE 32

// function prototype
int load_instructions();
void IF();
void ID();
void EX();
void MEM();
void WB();

struct Instruction {
    Instruction() {
        this->op = "null";  // op 初始為 null
    }

    Instruction(string op, int arg1, int arg2, int arg3) {
        this->op = op;
        this->rs = arg1;
        this->rt = arg2;

        if (op == "add" || op == "sub") {
            this->rd = arg3;
        } else if (op == "lw" || op == "sw" || op == "beq") {
            this->constant = arg3;
        }
    }

    // Data member
    string op;
    int rs;
    int rt;
    int rd;
    int constant;
};

// Pipeline register
struct IF_ID_pipeline {
    uint32_t PC;
    Instruction ins;
} IF_ID;

struct ID_EX_pipeline {
    // Seven Control Signals
    // EX
    bool RegDst;
    bool ALUSrc;

    // MEM
    bool Branch;
    bool MemRead;
    bool MemWrite;

    // WB
    bool RegWrite;
    bool MemToReg;

    // PC
    uint32_t PC;

    //
    uint32_t Read_data_1;
    uint32_t Read_data_2;

    // constant
    uint32_t offset;

    //
    uint32_t rs;
    uint32_t rt;
    uint32_t rd;

    string op;
} ID_EX;

struct EX_MEM_pipeline {
    // MEM
    bool Branch;
    bool MemRead;
    bool MemWrite;

    // WB
    bool RegWrite;
    bool MemToReg;

    uint32_t ALU_result;

    uint32_t Write_data;
    uint8_t Write_reg;
} EX_MEM;

struct MEM_WB_pipeline {
    // WB
    bool RegWrite;
    bool MemToReg;

    uint32_t Read_data;

    uint32_t ALU_result;

    uint8_t Write_reg;
} MEM_WB;

// Program counter
uint32_t PC;

uint32_t reg_file[REG_NUM];         // 32 個 register
uint32_t data_mem[DATA_MEM_SIZE];   // 32 個 words (data memory)
Instruction ins_mem[INS_MEM_SIZE];  // instruction memory

int main() {
    // 初始化 register
    reg_file[0] = 0;
    for (int i = 1; i < REG_NUM; i++) {
        reg_file[i] = 1;
    }

    // 初始化 memory
    for (int i = 0; i < DATA_MEM_SIZE; i++) {
        data_mem[i] = 1;
    }

    // 讀取指令至 instruction memory
    int ins_cnt = load_instructions();

    // 測試用
    // cout << endl;
    // for (int i = 0; i < ins_cnt; i++) {
    //     if (ins_mem[i].op == "add" || ins_mem[i].op == "sub")
    //         cout << ins_mem[i].op << " " << ins_mem[i].rs << " " << ins_mem[i].rt << " " << ins_mem[i].rd << endl;
    //     else
    //         cout << ins_mem[i].op << " " << ins_mem[i].rs << " " << ins_mem[i].rt << " " << ins_mem[i].constant << endl;
    // }

    // int cycle = 1;
    // do {
    //     WB();
    //     MEM();
    //     EX();
    //     ID();
    //     IF();

    //     cycle++;
    //     PC++;
    // } while (!(ins_mem[PC].op == "null" && IF_ID.ins.op == "null" && ID_EX.ins.op == "null"
    //             && EX_MEM.ins.op == "null" && MEM_WB.ins.op == "null"));

    return 0;
}

int load_instructions() {
    string line;
    int ins_cnt = 0;

    // 一行一行讀取指令
    while (getline(cin, line)) {
        // cout << line << endl;

        stringstream ss;
        ss << line;

        vector<string> tokens;
        string token;

        // 讀取指令名稱
        ss >> token;
        tokens.push_back(token);

        // 讀取參數
        char delimiter = ',';
        while (getline(ss, token, delimiter)) {
            // 去除頭尾空格
            int head = token.find_first_not_of(" ");
            int tail = token.find_last_not_of(" ");
            token = token.substr(head, tail - head + 1);

            tokens.push_back(token);
        }

        // 寫入 instruction memory
        if (tokens[0] == "add" || tokens[0] == "sub") {
            for (int i = 1; i <= 3; i++) {
                tokens[i] = tokens[i].substr(1, tokens[i].size());  // 取出 register 編號
            }

            ins_mem[ins_cnt++] = Instruction(tokens[0], stoi(tokens[2]), stoi(tokens[3]), stoi(tokens[1]));

        } else if (tokens[0] == "lw" || tokens[0] == "sw") {
            tokens[1] = tokens[1].substr(1, tokens[1].size());
            string tmp = tokens[2];
            int pos = tokens[2].find('(');
            tokens[2] = tmp.substr(0, pos);                               // 取出括號前的數字
            tokens.push_back(tmp.substr(pos + 2, tmp.size() - pos - 3));  // 取出 base register 編號

            ins_mem[ins_cnt++] = Instruction(tokens[0], stoi(tokens[3]), stoi(tokens[1]), stoi(tokens[2]));

        } else if (tokens[0] == "beq") {
            for (int i = 1; i <= 2; i++) {
                tokens[i] = tokens[i].substr(1, tokens[i].size());  // 取出 register 編號
            }

            ins_mem[ins_cnt++] = Instruction(tokens[0], stoi(tokens[1]), stoi(tokens[2]), stoi(tokens[3]));
        }
    }

    return ins_cnt;
}
