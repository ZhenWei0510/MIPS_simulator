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
void update_pipeline_register();

struct Instruction {
    Instruction() {
        this->op = "";  // op 初始為 nop
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
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
    uint32_t constant;
};

// Pipeline register
struct IF_ID_pipeline {
    uint32_t PC;
    Instruction ins;
} IF_ID, IF_tmp;

struct ID_EX_pipeline {
    // Seven Control Signals
    // EX
    char RegDst;
    char ALUSrc;

    // MEM
    char Branch;
    char MemRead;
    char MemWrite;

    // WB
    char RegWrite;
    char MemToReg;

    // PC
    uint32_t PC;

    //
    uint32_t Read_data_1;
    uint32_t Read_data_2;

    // constant
    uint32_t offset;

    //
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;

    string op;
} ID_EX, ID_tmp;

struct EX_MEM_pipeline {
    // MEM
    char Branch;
    char MemRead;
    char MemWrite;

    // WB
    char RegWrite;
    char MemToReg;

    uint32_t ALU_result;

    uint32_t Write_data;
    uint8_t Write_reg;

    string op;
} EX_MEM, EX_tmp;

struct MEM_WB_pipeline {
    // WB
    char RegWrite;
    char MemToReg;

    uint32_t Read_data;

    uint32_t ALU_result;

    uint8_t Write_reg;

    string op;
} MEM_WB, MEM_tmp;

// Program counter
uint32_t PC;

uint32_t reg_file[REG_NUM];         // 32 個 register
uint32_t data_mem[DATA_MEM_SIZE];   // 32 個 words (data memory)
Instruction ins_mem[INS_MEM_SIZE];  // instruction memory

bool IF_IDWrite = true;
bool PCWrite = true;

int cycle = 1;

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

    // 初始化 pipline registers's contorl signal
    ID_EX.RegDst = ID_EX.ALUSrc = ID_EX.Branch = ID_EX.MemRead = ID_EX.MemWrite = ID_EX.RegWrite = ID_EX.MemToReg = '0';
    EX_MEM.Branch = EX_MEM.MemRead = EX_MEM.MemWrite = EX_MEM.RegWrite = EX_MEM.MemToReg = '0';
    MEM_WB.RegWrite = MEM_WB.MemToReg = '0';

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

    do {
        cout << "Cycle " << cycle << endl;
        WB();
        MEM();
        EX();
        ID();
        IF();
        update_pipeline_register();
        cout << endl;

        cycle++;
        if (cycle == 50) break;
    } while (!(ins_mem[PC].op == "" && IF_ID.ins.op == "" && ID_EX.op == "" && EX_MEM.op == "" && MEM_WB.op == ""));

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

void IF() {
    cout << "  IF: " << ins_mem[PC].op << endl;
    IF_tmp.ins = ins_mem[PC];
    IF_tmp.PC = PC + 1;
    // cout << (int)IF_ID.ins.rs << " " << (int)IF_ID.ins.rt << " " << (int)IF_ID.ins.rd << endl;
}

void ID() {
    cout << "  ID: " << IF_ID.ins.op << endl;
    // Load-use hazard detection
    // cout << ID_EX.op << " " << ID_EX.MemRead << " " << (int)ID_EX.rt << " " << (int)IF_ID.ins.rs << " " << (int)IF_ID.ins.rt << endl;
    if (ID_EX.op != "" && ID_EX.MemRead == '1' && (ID_EX.rt == IF_ID.ins.rs || ID_EX.rt == IF_ID.ins.rt)) {
        ID_tmp.RegDst = ID_tmp.ALUSrc = '0';
        ID_tmp.Branch = ID_tmp.MemRead = ID_tmp.MemWrite = '0';
        ID_tmp.RegWrite = ID_tmp.MemToReg = '0';
        ID_tmp.op = "";
        IF_IDWrite = PCWrite = false;
        // cout << "test2";
        return;
    }
    if (IF_ID.ins.op == "add" || IF_ID.ins.op == "sub") {
        ID_tmp.RegDst = '1';
        ID_tmp.ALUSrc = '0';
        ID_tmp.Branch = '0';
        ID_tmp.MemRead = '0';
        ID_tmp.MemWrite = '0';
        ID_tmp.RegWrite = '1';
        ID_tmp.MemToReg = '0';
    } else if (IF_ID.ins.op == "lw") {
        ID_tmp.RegDst = '0';
        ID_tmp.ALUSrc = '1';
        ID_tmp.Branch = '0';
        ID_tmp.MemRead = '1';
        ID_tmp.MemWrite = '0';
        ID_tmp.RegWrite = '1';
        ID_tmp.MemToReg = '1';
    } else if (IF_ID.ins.op == "sw") {
        ID_tmp.RegDst = 'X';
        ID_tmp.ALUSrc = '1';
        ID_tmp.Branch = '0';
        ID_tmp.MemRead = '0';
        ID_tmp.MemWrite = '1';
        ID_tmp.RegWrite = '0';
        ID_tmp.MemToReg = 'X';
    } else if (IF_ID.ins.op == "beq") {
        // Load-use hazard detection
        // cout << ID_EX.op << " " << ID_EX.MemRead << " " << (int)ID_EX.rt << " " << (int)IF_ID.ins.rs << " " << (int)IF_ID.ins.rt << endl;
        // cout << EX_MEM.op << " " << EX_MEM.MemRead << " " << (int)EX_MEM.Write_reg << " " << (int)IF_ID.ins.rs << " " << (int)IF_ID.ins.rt << endl;
        if ((ID_EX.op != "" && ID_EX.MemRead == '1' && (ID_EX.rt == IF_ID.ins.rs || ID_EX.rt == IF_ID.ins.rt)) || (EX_MEM.op != "" && EX_MEM.MemRead == '1' && (EX_MEM.Write_reg == IF_ID.ins.rs || EX_MEM.Write_reg == IF_ID.ins.rt))) {
            ID_tmp.RegDst = ID_tmp.ALUSrc = '0';
            ID_tmp.Branch = ID_tmp.MemRead = ID_tmp.MemWrite = '0';
            ID_tmp.RegWrite = ID_tmp.MemToReg = '0';
            ID_tmp.op = "";
            IF_IDWrite = PCWrite = false;
            cout << "test1";
            return;
        }
        ID_tmp.RegDst = 'X';
        ID_tmp.ALUSrc = '0';
        ID_tmp.Branch = '1';
        ID_tmp.MemRead = '0';
        ID_tmp.MemWrite = '0';
        ID_tmp.RegWrite = '0';
        ID_tmp.MemToReg = 'X';
    }
    // if (MEM_WB.RegWrite && (MEM_WB.Write_reg != 0)
    //     && !(EX_MEM.RegWrite && (EX_MEM.Write_reg != 0)
    //         && (EX_MEM.Write_reg == ID_EX.rs))
    //     && (MEM_WB.Write_reg == ID_EX.rs)) {
    //     // ForwardA = 01
    // }

    IF_IDWrite = PCWrite = true;

    ID_tmp.PC = IF_ID.PC;

    ID_tmp.Read_data_1 = reg_file[IF_ID.ins.rs];
    ID_tmp.Read_data_2 = reg_file[IF_ID.ins.rt];

    ID_tmp.op = IF_ID.ins.op;

    ID_tmp.offset = IF_ID.ins.constant;

    ID_tmp.rs = IF_ID.ins.rs;
    ID_tmp.rt = IF_ID.ins.rt;
    ID_tmp.rd = IF_ID.ins.rd;
}

void EX() {
    cout << "  EX: " << ID_EX.op << endl;

    EX_tmp.Branch = ID_EX.Branch;
    EX_tmp.MemRead = ID_EX.MemRead;
    EX_tmp.MemWrite = ID_EX.MemWrite;
    EX_tmp.RegWrite = ID_EX.RegWrite;
    EX_tmp.MemToReg = ID_EX.MemToReg;

    EX_tmp.op = ID_EX.op;

    EX_tmp.ALU_result = 123;
    
    // // ALUOp
    // if (ID_EX.op == "add") {
    //     EX_MEM.ALU_result = ID_EX.Read_data_1 + ID_EX.Read_data_2;
    // } else if (ID_EX.op == "sub") {
    //     EX_MEM.ALU_result = ID_EX.Read_data_1 - ID_EX.Read_data_2;
    // } else if (ID_EX.op == "lw" || ID_EX.op == "sw") {
    //     EX_MEM.ALU_result = ID_EX.Read_data_1 + ID_EX.offset;
    // }
    
    // // ALU inputs
    // uint32_t input_1 = ID_EX.Read_data_1;
    // // Pass Read data 2 to Write_data
    // EX_tmp.Write_data = ID_EX.Read_data_2;

    // RegDst mux to choose rd or rt
    EX_MEM.Write_reg = ID_EX.RegDst == '0' ? ID_EX.rt : ID_EX.rd;
}

void MEM() {
    cout << "  MEM: " << EX_MEM.op << endl;

    MEM_tmp.RegWrite = EX_MEM.RegWrite;
    MEM_tmp.MemToReg = EX_MEM.MemToReg;

    if (EX_MEM.MemRead == '1') {
        // 執行讀取記憶體操作，將結果存入 MEM_WB 的 ALU_result
        MEM_tmp.Read_data = data_mem[EX_MEM.ALU_result];
    }
    if (EX_MEM.MemWrite == '1') {
        // 執行寫入記憶體操作
        data_mem[EX_MEM.ALU_result] = EX_MEM.Write_data;
    } else {
        MEM_tmp.ALU_result = EX_MEM.ALU_result;
    }

    MEM_tmp.Write_reg = EX_MEM.Write_reg;

    MEM_tmp.op = EX_MEM.op;
}

void WB() {
    cout << "  WB: " << MEM_WB.op << endl;

    if (MEM_WB.RegWrite == '1') {
        if (MEM_WB.MemToReg == '1') {
            reg_file[MEM_WB.Write_reg] = MEM_WB.Read_data;
        } else if (MEM_WB.MemToReg == '0') {
            reg_file[MEM_WB.Write_reg] = MEM_WB.ALU_result;
        }
    }
}

void update_pipeline_register() {
    if (IF_IDWrite) {
        IF_ID.ins = IF_tmp.ins;
    }
    if (PCWrite) {
        PC = IF_tmp.PC;
    }

    // ID_EX
    ID_EX.RegDst = ID_tmp.RegDst;
    ID_EX.ALUSrc = ID_tmp.ALUSrc;
    ID_EX.Branch = ID_tmp.Branch;
    ID_EX.MemRead = ID_tmp.MemRead;
    ID_EX.MemWrite = ID_tmp.MemWrite;
    ID_EX.RegWrite = ID_tmp.RegWrite;
    ID_EX.MemToReg = ID_tmp.MemToReg;
    //
    ID_EX.Read_data_1 = ID_tmp.Read_data_1;
    ID_EX.Read_data_2 = ID_tmp.Read_data_2;
    //
    ID_EX.PC = ID_tmp.PC;
    ID_EX.op = ID_tmp.op;
    //
    ID_EX.rs = ID_tmp.rs;
    ID_EX.rt = ID_tmp.rt;
    ID_EX.rd = ID_tmp.rd;
    ID_EX.offset = ID_tmp.offset;

    // EX_MEM
    EX_MEM.Branch = EX_tmp.Branch;
    EX_MEM.MemRead = EX_tmp.MemRead;
    EX_MEM.MemWrite = EX_tmp.MemWrite;
    EX_MEM.RegWrite = EX_tmp.RegWrite;
    EX_MEM.MemToReg = EX_tmp.MemToReg;
    EX_MEM.op = EX_tmp.op;
    //
    EX_MEM.ALU_result = EX_tmp.ALU_result;
    //
    EX_MEM.Write_data = EX_tmp.Write_data;
    //
    EX_MEM.Write_reg = EX_tmp.Write_reg;

    // MEM_WB
    MEM_WB.RegWrite = MEM_tmp.RegWrite;
    MEM_WB.MemToReg = MEM_tmp.MemToReg;
    MEM_WB.Read_data = MEM_tmp.Read_data;
    MEM_WB.ALU_result = MEM_tmp.ALU_result;
    MEM_WB.Write_reg = MEM_tmp.Write_reg;
    MEM_WB.op = MEM_tmp.op;
}