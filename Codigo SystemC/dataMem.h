#ifndef DATAMEN_H
#define DATAMEM_H

#include "mem.h"
#include "structsRV.h"
#include "systemc.h"

SC_MODULE(dataMem) {
public:
    sc_in<bool> clk, rst;

    sc_in<instruction> I;
    sc_out<instruction> instOut;


    mem *MEM;// público, muy poco elegante

    void registro();

    SC_CTOR(dataMem) {
        cout << "dataMem: " << name() << endl;

        INST.address = 0xffffffff;
        INST.I = 0x13;
        INST.aluOp = 0;
        INST.memOp = 15;
        INST.rs1 = INST.rs2 = INST.rd = 0x1f;//
        INST.wReg = false;
        INST.opA = INST.opB = INST.val2 = INST.aluOut = INST.dataOut = 0x0000dead;
        strcpy(INST.desc, "???");

        SC_METHOD(registro);
        sensitive << clk.pos();
    }

private:
    instruction INST;
};


#endif