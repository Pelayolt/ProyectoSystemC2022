#ifndef CACHEL2_H
#define CACHEL2_H

#include "mem.h"
#include "params.h"
#include "structsRV.h"
#include "systemc.h"
#include <cmath>
#include <deque>
#include <vector>

class coreRiscV;

SC_MODULE(cacheL2) {
public:
    sc_in<bool> clk, rst;

    sc_in<bool> fetch_req;
    sc_in<sc_uint<32>> fetch_addr;
    sc_out<bool> fetch_ready;
    sc_out<L2CacheLine> fetch_line_out;

    sc_in<bool> read_req;
    sc_in<bool> write_req;
    sc_in<sc_uint<32>> dataMem_addr;
    sc_in<sc_int<32>> dataMem_data;
    sc_out<bool> read_ready;
    sc_out<bool> write_ready;
    sc_out<L2CacheLine> dataMem_line_out;
    
    coreRiscV *instCore;

    mem *MEM;
    sc_module *backing_memory;

    SC_CTOR(cacheL2);

    void initCacheL2();
    void printCacheL2();

private:
    struct L2CacheSet {
        std::deque<L2CacheLine> ways;
    };

    std::vector<L2CacheSet> cache;
    unsigned current_lru = 0;

    enum ClientType { NONE, FETCH, DATAMEM_R, DATAMEM_W };
    ClientType client_pending = NONE;

    void updateLRU(L2CacheSet & set, L2CacheLine & accessedLine);
    void cacheL2_process();
    void writeLine(sc_uint<32> addr, const L2CacheLine &newline);
    void setWord(L2CacheLine & line, int idx, sc_int<32> value);
    sc_int<32> getWord(const L2CacheLine &line, int idx);

    // Internas
    int latency_counter = 0;
    L2CacheLine buffer_out;
    sc_uint<32> addr_buf = 0;
    bool pending_response = false;
    bool notifying_to_dataMem = false;
    bool notifying_to_fetch = false;

    bool write_pending = false;
    int write_counter = 0;
    bool write_miss = false;
    sc_uint<32> w_addr;
    sc_int<32> w_data;
    unsigned w_offset;
};
#endif