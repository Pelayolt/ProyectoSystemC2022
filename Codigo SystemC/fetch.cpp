#include "fetch.h"
#include <iostream>
#include <iomanip>

extern FILE *fout1;


SC_HAS_PROCESS(fetch);

fetch::fetch(sc_module_name name) : sc_module(name) {
    std::cout << "fetch: " << name << std::endl;

    INST.address = 0xffffffff;
    INST.I = 0x13;
    INST.aluOp = 0;
    INST.memOp = 15;
    INST.rs1 = INST.rs2 = INST.rd = 0x1f;
    INST.wReg = false;
    INST.opA = INST.opB = INST.val2 = INST.aluOut = INST.dataOut = 0x0000dead;
    strcpy(INST.desc, "???");

    SC_METHOD(updatePC);
    sensitive << hazard << bubble << PCext << fire;

    SC_METHOD(registro);
    sensitive << clk.pos();

    initCache();
}

void fetch::initCache() {
    cache.resize(NUMLINES_L1_I);
    for (auto &set: cache) {
        set.ways.resize(ASSOCIATIVITY_L1_I);
        for (auto &line: set.ways) {
            line.valid = false;
            line.tag = 0;
            line.data.resize(WORDSPERLINE_L1_I, 0xDEADBEEF);
        }
    }
}

void fetch::updatePC() {

    sc_uint<32> tmp;

    tmp = PCext.read();

    tiempo = sc_time_stamp().to_double() / 1000.0;
    if (bubble.read())
        newPC = PCext.read();
    else if (hazard.read())
        newPC = PC;
    else {
        newPC = PC + 4;
    }
}

void fetch::startL2Request(sc_uint<32> addr) {
    addr_buf = addr;
    addr_cacheL2.write(addr & ~(WORDSPERLINE_L2 * 4 - 1));
    req_cacheL2.write(true);
    state = WAIT_L2;
}

bool fetch::isL2RequestComplete() {
    if (ready_cacheL2.read()) {
        l2_line_buf = line_in.read();
        req_cacheL2.write(false);
        return true;
    }
    return false;
}

void fetch::storeLineToL1() {
    sc_uint<32> index = (addr_buf >> 2) / WORDSPERLINE_L1_I % NUMLINES_L1_I;
    sc_uint<32> tag = addr_buf >> (2 + int(log2(WORDSPERLINE_L1_I)) + int(log2(NUMLINES_L1_I)));
    unsigned offset = ((addr_buf >> 2) & (WORDSPERLINE_L2 - 1)) / WORDSPERLINE_L1_I;

    std::vector<sc_uint<32>> lineaL1(WORDSPERLINE_L1_I);
    for (unsigned i = 0; i < WORDSPERLINE_L1_I; ++i) {
        unsigned l2_idx = offset * WORDSPERLINE_L1_I + i;
        if (l2_idx < WORDSPERLINE_L2)
            lineaL1[i] = l2_line_buf.data[l2_idx];
        else
            lineaL1[i] = 0xDEADBEEF; // Relleno si el índice se sale del rango
    }

    instCacheLine newline;
    newline.valid = true;
    newline.tag = tag;
    newline.data = lineaL1;
    newline.lru_counter = current_lru++;

    CacheSet &set = cache[index];
    if (set.ways.size() >= ASSOCIATIVITY_L1_I) {
        if (USEFIFO_L1_I)
            set.ways.pop_front();
        else {
            auto lru_it = std::min_element(set.ways.begin(), set.ways.end(),
                                           [](const instCacheLine &a, const instCacheLine &b) {
                                               return a.lru_counter < b.lru_counter;
                                           });
            set.ways.erase(lru_it);
        }
    }

    set.ways.push_back(newline);
    state = IDLE;
}

sc_uint<32> fetch::fetchFromCache(sc_uint<32> addr, bool &isHit) {
    sc_uint<32> index = (addr >> 2) / WORDSPERLINE_L1_I % NUMLINES_L1_I;
    sc_uint<32> tag = addr >> (2 + int(log2(WORDSPERLINE_L1_I)) + int(log2(NUMLINES_L1_I)));
    sc_uint<32> offset = (addr >> 2) % WORDSPERLINE_L1_I;

    CacheSet &set = cache[index];
    for (auto &line: set.ways) {
        if (line.valid && line.tag == tag) {
            isHit = true;
            return line.data[offset];
        }
    }

    isHit = false;
    return 0;
}

void fetch::registro() {
    sc_uint<32> tmp = PCext.read();
    tiempo = sc_time_stamp().to_double() / 1000.0;
    if (PRINT) fprintf(fout1, ";COUNTERS;%.0f;", tiempo);
    
    if (rst.read()) {
        PC = 0;
        INST.I = 0x00000013;
        INST.address = PC;
        PCout.write(PC);
        instOut.write(INST);
        numInst = 0;
        state = IDLE;
        reqToL2 = false;
        return;
    }

    if (PRINT) fprintf(fout1, "0x%08X;CACHE INSTR;", static_cast<unsigned int>(PC));

    switch (state) {
        case IDLE: {
            if (bubble.read()) {
                INST.I = 0x00000013;
                INST.address = PC;
                PCout.write(PC);
                PC = newPC;
                if (PRINT) fprintf(fout1, "Burbuja");
            } else if (hazard.read()) {
                PC = newPC;
                if (PRINT) fprintf(fout1, "Riesgo");
            } else {
                bool hit;
                sc_uint<32> instVal = fetchFromCache(PC, hit);

                if (hit) {
                    INST.I = instVal;
                    INST.address = PC;
                    PCout.write(PC);
                    ++numInst;
                    cache_hits++;
                    if (PRINT) fprintf(fout1, "Instruccion 0x%08X encontrada", static_cast<unsigned int>(INST.I));
                    PC = newPC;
                } else {
                    cache_misses++;
                    INST.I = 0x00000013;
                    INST.address = PC;
                    PCout.write(PC);
                    ++numInst;
                    startL2Request(PC);
                    if (PRINT) fprintf(fout1, "Fallo en cache, solicitando instruccion a cacheL2");
                }
            }
            break;

        }

        case WAIT_L2: {
            if (isL2RequestComplete()) {
                storeLineToL1();
                if (PRINT) fprintf(fout1, "Instruccion recibida y almacenada en cache");
            } else {    
                if (PRINT) fprintf(fout1, "Esperando a que cacheL2 envie la instruccion");
            }
            INST.I = 0x00000013;
            INST.address = PC;
            PCout.write(PC);
            ++numInst;

            break;
        }

    }
    instOut.write(INST);
    fire.write(!fire.read());
}

void fetch::end_of_simulation() {
    std::cout << "\n=== Estadisticas de cacheL1I ===\n";
    std::cout << "Vias: " << dec << ASSOCIATIVITY_L1_I << std::endl;
    std::cout << "Number of lines: " << dec << NUMLINES_L1_I << std::endl;
    std::cout << "Words per line: " << dec << WORDSPERLINE_L1_I << std::endl;
    if (USEFIFO_L1_I)
        std::cout << "Used FIFO" << std::endl;
    else
        std::cout << "Used LRU" << std::endl;
    std::cout << "Hits:   " << dec << cache_hits << std::endl;
    std::cout << "Misses: " << dec << cache_misses << std::endl;
    std::cout << "Tasa de acierto: " << dec << 100.0 * cache_hits / (cache_hits + cache_misses) << "%\n";
}
