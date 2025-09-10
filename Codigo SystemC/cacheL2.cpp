#include "cacheL2.h"
#include <algorithm>
#include <iomanip>


SC_HAS_PROCESS(cacheL2);

cacheL2::cacheL2(sc_module_name name) : sc_module(name) {
    initCacheL2();

    SC_METHOD(cacheL2_process);
    sensitive << clk.pos();
    dont_initialize();

    latency_counter = LATENCY_CYCLES_L2;
}

void cacheL2::initCacheL2() {
    cache.resize(NUMLINES_L2);
    for (auto &set: cache) {
        set.ways.clear();
    }
}

void cacheL2::cacheL2_process() {
    if (rst.read()) {
        latency_counter = LATENCY_CYCLES_L2;
        fetch_ready.write(false);
        write_ready.write(false);
        read_ready.write(false);
        pending_response = false;
        client_pending = NONE;
        notifying_to_dataMem = false;
        notifying_to_fetch = false;
        return;
    }

    fetch_ready.write(false);
    write_ready.write(false);
    read_ready.write(false);

    if (PRINT) cout << ";CACHE L2;";

    if (!pending_response) {
        if (write_req.read() && !notifying_to_dataMem) {
            addr_buf = dataMem_addr.read();
            client_pending = DATAMEM_W;
        } else if (read_req.read() && !notifying_to_dataMem) {
            addr_buf = dataMem_addr.read();
            client_pending = DATAMEM_R;
        } else if (fetch_req.read() && !notifying_to_fetch) {
            addr_buf = fetch_addr.read();
            client_pending = FETCH;
        } else {
            client_pending = NONE;
        }

        if (client_pending == FETCH || client_pending == DATAMEM_R) {
            // --- BLOQUE DE LECTURA ---

            sc_uint<32> index = (addr_buf >> 2) / WORDSPERLINE_L2 % NUMLINES_L2;
            sc_uint<32> tag = addr_buf >> (2 + int(log2(WORDSPERLINE_L2)) + int(log2(NUMLINES_L2)));

            L2CacheSet &set = cache[index];
            bool hit = false;

            for (auto &line: set.ways) {
                if (line.valid && line.tag == tag) {
                    for (int i = 0; i < WORDSPERLINE_L2; ++i)
                        setWord(buffer_out, i, getWord(line, i));

                    buffer_out.valid = true;
                    buffer_out.tag = line.tag;
                    buffer_out.lru_counter = line.lru_counter;

                    updateLRU(set, line);
                    hit = true;
                    break;
                }
            }

            if (!hit) {
                sc_uint<32> baseAddr = addr_buf & ~(WORDSPERLINE_L2 * 4 - 1);
                L2CacheLine newline;
                newline.valid = true;
                newline.tag = tag;
                newline.lru_counter = current_lru++;

                for (unsigned i = 0; i < WORDSPERLINE_L2; ++i) {
                    sc_uint<32> currAddr = baseAddr + i * 4;
                    sc_int<32> word = MEM->isValidAddress(currAddr) ? MEM->readWord(currAddr) : 0;

                    setWord(newline, i, word);
                    setWord(buffer_out, i, word);
                }

                buffer_out.valid = true;
                buffer_out.tag = tag;
                buffer_out.lru_counter = newline.lru_counter;

                writeLine(addr_buf, newline);
                
                latency_counter = LATENCY_CYCLES_L2 + LATENCY_CYCLES_MEM;
                if (PRINT && client_pending == FETCH) 
                    cout << "Fallo en cache, esperando instruccion a cache (" << LATENCY_CYCLES_L2 << " clk) + mem (" << LATENCY_CYCLES_MEM << " clk);";
                else if (PRINT && client_pending == DATAMEM_R)
                    cout << "Fallo en cache, esperando dato a cache (" << LATENCY_CYCLES_L2 << " clk) + mem (" << LATENCY_CYCLES_MEM << " clk);";
            } else {
                latency_counter = LATENCY_CYCLES_L2;
                if (PRINT && client_pending == FETCH) cout << "Acierto en cache, esperando instruccion a cache (" << LATENCY_CYCLES_L2 << " clk);";
                else if (PRINT && client_pending == DATAMEM_R) cout << "Acierto en cache, esperando dato a cache (" << LATENCY_CYCLES_L2 << " clk);";
            }

            pending_response = true;
        } else if (client_pending == DATAMEM_W) {
            // --- BLOQUE DE ESCRITURA ---

            sc_int<32> data = dataMem_data.read();
            unsigned word_offset = (addr_buf >> 2) % WORDSPERLINE_L2;

            sc_uint<32> index = (addr_buf >> 2) / WORDSPERLINE_L2 % NUMLINES_L2;
            sc_uint<32> tag = addr_buf >> (2 + int(log2(WORDSPERLINE_L2)) + int(log2(NUMLINES_L2)));

            L2CacheSet &set = cache[index];

            bool hit = false;
            for (auto &line: set.ways) {
                if (line.valid && line.tag == tag) {
                    setWord(line, word_offset, data);
                    line.dirty = true;
                    updateLRU(set, line);
                    hit = true;
                    break;
                }
            }

            // Miss: política write-allocate
            if (!hit) {
                L2CacheLine newline;
                newline.valid = true;
                newline.tag = tag;
                newline.lru_counter = current_lru++;

                for (unsigned i = 0; i < WORDSPERLINE_L2; ++i) {
                    sc_uint<32> currAddr = (addr_buf & ~(WORDSPERLINE_L2 * 4 - 1)) + i * 4;
                    sc_int<32> mem_word = MEM->isValidAddress(currAddr) ? MEM->readWord(currAddr) : 0;
                    sc_int<32> word = (i == word_offset) ? data : (sc_int<32>) mem_word;

                    setWord(newline, i, word);
                }

                writeLine(addr_buf, newline);
                latency_counter = LATENCY_CYCLES_L2 + LATENCY_CYCLES_MEM;
                if (PRINT) cout << "Fallo en cache, esperando dato a cache y escritura a mem (" << LATENCY_CYCLES_L2 << " clk) + mem (" << LATENCY_CYCLES_MEM << " clk);";
            } else {
                latency_counter = LATENCY_CYCLES_L2;
                if (PRINT) cout << "Acierto en cache, esperando escritura a mem (" << LATENCY_CYCLES_L2 << " clk);";
            }
            //latency_counter = 1;
            MEM->writeWord(addr_buf, data);// Write-through
            pending_response = true;

        } else if (PRINT)
            cout << ";";
    }

    notifying_to_dataMem = false;
    notifying_to_fetch = false;

    // --- RESPUESTA ---
    if (pending_response && --latency_counter <= 0) {
        if (client_pending == FETCH) {
            fetch_line_out.write(buffer_out);
            fetch_ready.write(true);
            notifying_to_fetch = true;
            if (PRINT) cout << "Instruccion almacenada en cache";
        } else if (client_pending == DATAMEM_R) {
            dataMem_line_out.write(buffer_out);
            read_ready.write(true);
            notifying_to_dataMem = true;
            if (PRINT) cout << "Dato almacenado en cache";
        } else if (client_pending == DATAMEM_W) {
            dataMem_line_out.write(buffer_out);
            write_ready.write(true);
            notifying_to_dataMem = true;
            if (PRINT) cout << "Dato guardado en memoria";
        }

        pending_response = false;
        client_pending = NONE;
        latency_counter = LATENCY_CYCLES_L2;
    }
}

void cacheL2::writeLine(sc_uint<32> addr, const L2CacheLine &newline) {
    sc_uint<32> index = (addr >> 2) / WORDSPERLINE_L2 % NUMLINES_L2;
    L2CacheSet &set = cache[index];

    // Reemplazo si es necesario
    if (set.ways.size() >= ASSOCIATIVITY_L2) {
        auto it = USEFIFO_L2 ? set.ways.begin()
                             : std::min_element(set.ways.begin(), set.ways.end(),
                                                [](const L2CacheLine &a, const L2CacheLine &b) {
                                                    return a.lru_counter < b.lru_counter;
                                                });

        L2CacheLine &victim = *it;

        if (victim.dirty) {
            // Calcular dirección base de la línea víctima
            sc_uint<32> victim_tag = victim.tag;
            sc_uint<32> base_addr =
                    (victim_tag << (int(log2(WORDSPERLINE_L2)) + int(log2(NUMLINES_L2)))) |
                    (index << int(log2(WORDSPERLINE_L2)));
            base_addr <<= 2;

            // Escribir cada palabra a memoria
            for (unsigned i = 0; i < WORDSPERLINE_L2; ++i) {
                sc_int<32> word = getWord(victim, i);
                MEM->writeWord(base_addr + 4 * i, word);
            }
        }

        // Eliminar línea víctima
        set.ways.erase(it);
    }

    // Insertar nueva línea
    set.ways.push_back(newline);
    //printCacheL2();
}

void cacheL2::printCacheL2() {
    std::cout << "\n============================================================ CACHE L2 ============================================================\n";
    std::cout << " Index | Way |  Valid  |  Dirty |      Tag      |                                       Data\n";
    std::cout << "----------------------------------------------------------------------------------------------------------------------------------\n";

    for (unsigned index = 0; index < NUMLINES_L2; ++index) {
        const L2CacheSet &set = cache[index];
        for (unsigned way = 0; way < set.ways.size(); ++way) {
            const L2CacheLine &line = set.ways[way];

            std::cout << std::hex << std::setw(6) << index << " | "
                      << std::hex << std::setw(3) << way << " | "
                      << "   " << line.valid << "    |   "
                      << line.dirty << "    |  0x"
                      << std::setw(6) << std::setfill('0') << line.tag << "  | ";

            for (unsigned i = 0; i < WORDSPERLINE_L2; ++i) {
                sc_int<32> word = getWord(line, i);
                std::cout << std::setw(8) << std::setfill('0') << word << " ";
            }

            std::cout << std::dec << std::endl;
        }
    }

    std::cout << "==================================================================================================================================\n\n";
}



sc_int<32> cacheL2::getWord(const L2CacheLine &line, int idx) {
    return sc_int<32>(line.data.range((idx + 1) * 32 - 1, idx * 32).to_uint());
}

void cacheL2::setWord(L2CacheLine &line, int idx, sc_int<32> value) {
    line.data.range((idx + 1) * 32 - 1, idx * 32) = value;
}

void cacheL2::updateLRU(L2CacheSet &set, L2CacheLine &accessedLine) {
    if (!USEFIFO_L2) {
        accessedLine.lru_counter = current_lru++;
    }
}
