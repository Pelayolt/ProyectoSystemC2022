#ifndef DECOD_H
#define DECOD_H

#include "structsRV.h"
#include "systemc.h"

SC_MODULE(decod) {
public:
    sc_in<bool> clk;
    sc_in<bool> rst;

    sc_in<instruction> inst;
    sc_in<sc_uint<32>> PCin;
    sc_in<instruction> fbEx, fbMem, fbWB;

    sc_out<instruction> instOut;

    sc_out<bool> hazard, bubble;
    sc_out<sc_uint<32>> PCout;

    void decoding();
    void registros();

    SC_CTOR(decod) {
        cout << "decod: " << name() << endl;
        fire.write(0);

        INST.address = 0xffffffff;
        INST.I = 0x13;
        INST.aluOp = 0;
        INST.memOp = 15;
        INST.rs1 = INST.rs2 = INST.rd = 0x1f;//
        INST.wReg = false;
        INST.opA = INST.opB = INST.val2 = INST.aluOut = INST.dataOut = 0x0000dead;
        strcpy(INST.desc, "???");

        SC_METHOD(decoding);
        sensitive << fire;
        SC_METHOD(registros);
        sensitive << clk.pos();
    }

    unsigned int *numInst;

private:
    sc_int<32> regs[32];
    sc_signal<bool> fire;
    instruction INST;

    sc_uint<5> C_rd;
    bool C_wReg;
    sc_uint<5> C_aluOp;
    sc_uint<4> C_memOp;
    sc_int<32> C_opA, C_opB, C_rs2;

    double tiempo;
};

#endif
