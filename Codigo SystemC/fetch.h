#ifndef FETCH_H
#define FETCH_H

#include "mem.h"
#include "structsRV.h"
#include "systemc.h"


SC_MODULE(fetch) {
public:
    sc_in<bool> clk, rst;

    sc_in<sc_uint<32>> PCext;
    sc_in<bool> hazard, bubble;

    sc_out<instruction> instOut;
    sc_out<sc_uint<32>> PCout;

    mem *MEM;// público, muy poco elegante

    void registro();
    void updatePC();
    void initPC(int initVal) { PC = initVal; }

    SC_CTOR(fetch) {
        cout << "fetch: " << name() << endl;

        INST.address = 0xffffffff;
        INST.I = 0x13;
        INST.aluOp = 0;
        INST.memOp = 15;
        INST.rs1 = INST.rs2 = INST.rd = 0x1f;//
        INST.wReg = false;
        INST.opA = INST.opB = INST.val2 = INST.aluOut = INST.dataOut = 0x0000dead;
        strcpy(INST.desc, "???");


        SC_METHOD(updatePC);
        sensitive << hazard << bubble << PCext << fire;

        SC_METHOD(registro);
        sensitive << clk.pos();
    }

    unsigned int numInst;

private:
    sc_uint<32> PC, newPC;
    sc_signal<bool> fire;
    instruction INST;

    double tiempo;
};


#endif