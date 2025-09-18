#ifndef DATAMEM_H
#define DATAMEM_H

#include "mem.h"
#include "structsRV.h"
#include "params.h"
#include "systemc.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <vector>
#include <queue>
#include <bitset>

SC_MODULE(dataMem) {
public:
    sc_in<bool> clk, rst;
    sc_in<instruction> I;
    sc_out<instruction> instOut;
    sc_out<sc_uint<32>> pendingRdMask;
    sc_out<sc_uint<32>> queueAvailableSpace;

    sc_in<bool> write_ready_cacheL2;
    sc_in<bool> read_ready_cacheL2;
    sc_in<L2CacheLine> line_in;
    sc_out<sc_int<32>> data_cacheL2;
    sc_out<sc_uint<32>> addr_cacheL2;
    sc_out<bool> read_req_cacheL2;    
    sc_out<bool> write_req_cacheL2;


    // Acceso a memoria directa (solo para Debug)
    mem *MEM;

    void registro();

    SC_CTOR(dataMem) {
        std::cout << "dataMem: " << name() << std::endl;

        INST.address = 0xffffffff;
        INST.I = 0x13;
        INST.aluOp = 0;
        INST.memOp = 15;
        INST.rs1 = INST.rs2 = INST.rd = 0x1f;
        INST.wReg = false;
        INST.opA = INST.opB = INST.val2 = INST.aluOut = INST.dataOut = 0x0000dead;
        std::strcpy(INST.desc, "???");

        current_lru = 0;
        cache.resize(NUMLINES_L1_D);

        SC_METHOD(registro);
        sensitive << clk.pos();

    }

private:
    instruction INST;
    
    struct CacheSet {
        std::deque<dataCacheLine> ways;
    };
    std::vector<CacheSet> cache;
    std::queue<instruction> pendingQueue;

    unsigned current_lru;
    bool waitingL2 = false;
    bool pendingWriteL2 = false;
    sc_uint<32> addr_buf;
    L2CacheLine l2_line_buf;
    double tiempo;

    void initCache();
    void updatePendingMask();
    bool accessCache(sc_int<32> addr, sc_int<32> & word, bool isWrite, sc_int<32> writeData);
    void storeLineToL1(sc_uint<32> addr, const L2CacheLine &line);
    void startL2Request(sc_int<32> addr, bool isWrite, sc_int<32> writeData);
    bool isL2RequestCompleteR();
    bool isL2RequestCompleteW();
    void addNewInst(instruction i);
    void startL2Write(sc_uint<32> addr, sc_int<32> writeData);
    sc_int<32> decodeReadData(sc_uint<4> op, sc_int<32> word, int BH);
    sc_uint<32> getWord(const L2CacheLine &line, int idx);
    void printPendings();

    };

#endif
