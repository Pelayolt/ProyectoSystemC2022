#ifndef FETCH_H
#define FETCH_H

#include "cacheL2.h"
#include "params.h"
#include "systemc.h"
#include "structsRV.h"
#include <deque>
#include <vector>

SC_MODULE(fetch) {
public:
    sc_in<bool> clk, rst;
    sc_in<sc_uint<32>> PCext;
    sc_in<bool> hazard, bubble;
    sc_out<instruction> instOut;
    sc_out<sc_uint<32>> PCout;
   
    sc_in<bool> ready_cacheL2;
    sc_in<L2CacheLine> line_in;
    sc_out<sc_uint<32>> addr_cacheL2;
    sc_out<bool> req_cacheL2;

    cacheL2 *instCacheL2;

    void registro();
    void updatePC();
    void initPC(int initVal) { PC = initVal; }
    virtual void end_of_simulation();


    SC_CTOR(fetch);

    unsigned int numInst;
    unsigned cache_hits = 0;
    unsigned cache_misses = 0;

private:
    sc_uint<32> PC, newPC;
    sc_signal<bool> fire;
    instruction INST;
    double tiempo;

    enum FetchState { IDLE,
                      WAIT_L2 };

    struct CacheSet {
        std::deque<instCacheLine> ways;
    };

    std::vector<CacheSet> cache;
    unsigned int current_lru = 0;
    FetchState state = IDLE;

    L2CacheLine l2_line_buf;
    sc_uint<32> addr_buf;
    bool reqToL2 = false;

    void initCache();
    sc_uint<32> fetchFromCache(sc_uint<32> addr, bool &isHit);
    void startL2Request(sc_uint<32> addr);
    bool isL2RequestComplete();
    void storeLineToL1();
    void printCacheL1Instr();
};

#endif
