#ifndef CORERISCV_H
#define CORERISCV_H

#include "alu.h"
#include "dataMem.h"
#include "decod.h"
#include "fetch.h"
#include "mem.h"
#include "structsRV.h"
#include "systemc.h"
#include <stdio.h>

SC_MODULE(coreRiscV) {
public:
    sc_in<bool> clk, rst;

    void writeInst(int addr, int data) { MEM->writeWord(addr, data); };
    void writeWord(int addr, int data) { MEM->writeWord(addr, data); };
    sc_int<32> readWord(int addr) { return MEM->readWord(addr); };
    int leeELF(FILE * elf);


    SC_CTOR(coreRiscV) {
        cout << "coreRiscV: " << name() << endl;

        instFetch = new fetch("instFetch");
        instDecod = new decod("instDecod");
        instAlu = new alu("instAlu");
        instDataMem = new dataMem("instDataMem");


        instFetch->clk(clk);
        instFetch->rst(rst);
        instFetch->PCext(PC_DecodFetch);
        instFetch->hazard(hazard);
        instFetch->bubble(bubble);
        instFetch->instOut(iFD);
        instFetch->PCout(PC_FetchDecod);

        instDecod->clk(clk);
        instDecod->rst(rst);
        instDecod->inst(iFD);
        instDecod->PCin(PC_FetchDecod);

        instDecod->hazard(hazard);
        instDecod->bubble(bubble);

        instDecod->fbEx(iDX);
        instDecod->fbMem(iXM);
        instDecod->fbWB(iMW);
        instDecod->instOut(iDX);
        instDecod->PCout(PC_DecodFetch);

        instAlu->clk(clk);
        instAlu->rst(rst);
        instAlu->I(iDX);
        instAlu->instOut(iXM);

        instDataMem->clk(clk);
        instDataMem->rst(rst);
        instDataMem->I(iXM);
        instDataMem->instOut(iMW);


        MEM = new mem;
        instFetch->MEM = MEM;
        instDataMem->MEM = MEM;

        instDecod->numInst = &(instFetch->numInst);
    }

private:
    fetch *instFetch;
    decod *instDecod;
    alu *instAlu;
    dataMem *instDataMem;

    mem *MEM;

    sc_signal<sc_uint<32>> PC_DecodFetch, PC_FetchDecod;
    sc_signal<bool> hazard, bubble;

    sc_signal<instruction> iFD, iDX, iXM, iMW;
    /*	sc_signal< sc_int<32> >		wbValue, opA, opB, rs2_DescodAlu, rs2_AluDataMem;
	sc_signal< sc_uint<5> >		mRegEX, mRegMem, mRegWB;
	sc_signal<bool>				wRegEX, wRegMem, wRegWB;

	sc_signal< sc_uint<4> >		aluOp_DescodAlu, memOp_DescodAlu, memOp_AluDataMem;
	sc_signal< sc_int<32> >		aluOut;	*/
};

#endif