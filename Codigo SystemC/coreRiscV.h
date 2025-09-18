#ifndef CORERISCV_H
#define CORERISCV_H

#include <stdio.h>
#include "systemc.h"
#include "cacheL2.h"
#include "fetch.h"
#include "decod.h"
#include "alu.h"
#include "dataMem.h"
#include "mem.h"
#include "structsRV.h"

SC_MODULE(coreRiscV) {
public:

	sc_in<bool> clk, rst;

	void writeInst(int addr, int data) { MEM->writeWord(addr, data); };
	void writeWord(int addr, int data) { MEM->writeWord(addr, data); };
	sc_int<32> readWord(int addr) { return MEM->readWord(addr);	};
	int leeELF(FILE* elf);

	SC_CTOR(coreRiscV) {
		cout << "coreRiscV: " << name() << endl;

		MEM = new mem;
		instFetch = new fetch("instFetch");
		instDecod = new decod("instDecod");
		instAlu = new alu("instAlu");
		instDataMem = new dataMem("instDataMem");
        instCacheL2 = new cacheL2("instCacheL2");

		instCacheL2->clk(clk);
        instCacheL2->rst(rst);
        instCacheL2->fetch_req(req_L2_fetch);
        instCacheL2->fetch_addr(addr_L2_fetch);
        instCacheL2->fetch_ready(ready_L2_fetch);
        instCacheL2->fetch_line_out(line_L2_fetch);
        instCacheL2->read_req(req_L2_dataMem);
        instCacheL2->write_req(write_req);
        instCacheL2->dataMem_addr(addr_L2_dataMem);
        instCacheL2->write_ready(write_ready);
        instCacheL2->read_ready(ready_L2_dataMem);
        instCacheL2->dataMem_line_out(line_L2_dataMem);
        instCacheL2->dataMem_data(write_data);
		
		instFetch->clk(clk);
		instFetch->rst(rst);
		instFetch->PCext(PC_DecodFetch);
		instFetch->hazard(hazard);
		instFetch->bubble(bubble);
		instFetch->instOut(iFD);
		instFetch->PCout(PC_FetchDecod);
        instFetch->req_cacheL2(req_L2_fetch);
        instFetch->ready_cacheL2(ready_L2_fetch);
        instFetch->addr_cacheL2(addr_L2_fetch);
        instFetch->line_in(line_L2_fetch);

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
        instDecod->pendingRdMask(Mask);
        instDecod->queueAvailableSpace(QS);
		instDecod->PCout(PC_DecodFetch);

		instAlu->clk(clk);
		instAlu->rst(rst);
		instAlu->I(iDX);
		instAlu->instOut(iXM);

		instDataMem->clk(clk);
		instDataMem->rst(rst);
		instDataMem->I(iXM);
		instDataMem->instOut(iMW);
        instDataMem->pendingRdMask(Mask);
        instDataMem->queueAvailableSpace(QS);

        instDataMem->read_req_cacheL2(req_L2_dataMem);
        instDataMem->write_req_cacheL2(write_req);
        instDataMem->write_ready_cacheL2(write_ready);
        instDataMem->read_ready_cacheL2(ready_L2_dataMem);
        instDataMem->addr_cacheL2(addr_L2_dataMem);
        instDataMem->line_in(line_L2_dataMem);
        instDataMem->line_out(write_data);

		instFetch->instCacheL2 = instCacheL2;
        instCacheL2->MEM = MEM;
		
		instDataMem->MEM = MEM;
		instDecod->numInst = &(instFetch->numInst);

	}

private:

	fetch* instFetch;
	decod* instDecod;
	alu* instAlu;
	dataMem* instDataMem;
    cacheL2 *instCacheL2;

	mem* MEM; 

	sc_signal< sc_uint<32> >	PC_DecodFetch, PC_FetchDecod;
	sc_signal< bool >			hazard, bubble;

    sc_signal<bool> req_L2_fetch;
    sc_signal<bool> ready_L2_fetch;
    sc_signal<sc_uint<32>> addr_L2_fetch;
    sc_signal<L2CacheLine> line_L2_fetch;
    
    sc_signal<bool> req_L2_dataMem;
    sc_signal<bool> ready_L2_dataMem;
    sc_signal<sc_uint<32>> addr_L2_dataMem;
    sc_signal<L2CacheLine> line_L2_dataMem;

    sc_signal<bool> write_req;
    sc_signal<bool> write_ready;
    sc_signal<dataCacheLine> write_data;
    sc_signal<sc_uint<32>> write_addr;

	sc_signal<sc_uint<32>> Mask;
    sc_signal<sc_uint<32>> QS;

	sc_signal < instruction >	iFD, iDX, iXM, iMW; 

    /*	sc_signal< sc_int<32> >		wbValue, opA, opB, rs2_DescodAlu, rs2_AluDataMem;
	sc_signal< sc_uint<5> >		mRegEX, mRegMem, mRegWB;
	sc_signal<bool>				wRegEX, wRegMem, wRegWB;

	sc_signal< sc_uint<4> >		aluOp_DescodAlu, memOp_DescodAlu, memOp_AluDataMem;
	sc_signal< sc_int<32> >		aluOut;	*/

};

#endif