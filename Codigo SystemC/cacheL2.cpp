#include "cacheL2.h"
#include <algorithm>
#include <iomanip>

extern FILE *fout1;
extern FILE *fout6;


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
    tiempo = sc_time_stamp().to_double() / 1000.0;

    if (rst.read()) {
        latency_counter = LATENCY_CYCLES_L2;
        fetch_ready.write(false);
        write_ready.write(false);
        read_ready.write(false);
        pending_response = false;
        client_pending = NONE;
        notifying_to_dataMem = false;
        notifying_to_fetch = false;
        sharedFlag = 3;
        return;
    }

    if (PRINT && sharedFlag < 3) printCacheL2();


    fetch_ready.write(false);
    write_ready.write(false);
    read_ready.write(false);

    if (PRINT) fprintf(fout1, ";CACHE L2;");

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
                        buffer_out.data[i] = line.data[i];

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
                    sc_int<32> word = MEM->isValidAddress(currAddr) ? MEM->readWord(currAddr) : 0x0000dead;

                    newline.data[i] = word;
                    buffer_out.data[i] = word;
                }

                buffer_out.valid = true;
                buffer_out.tag = tag;
                buffer_out.lru_counter = newline.lru_counter;

                writeLine(addr_buf, newline);
                
                latency_counter = LATENCY_CYCLES_L2 + LATENCY_CYCLES_MEM;
                if (PRINT && client_pending == FETCH) 
                    fprintf(fout1, "Fallo en cache, esperando instruccion a cache (%d clk) + mem (%d clk)", LATENCY_CYCLES_L2, LATENCY_CYCLES_MEM);
                else if (PRINT && client_pending == DATAMEM_R)
                    fprintf(fout1, "Fallo en cache, esperando dato a cache (%d clk) + mem (%d clk)", LATENCY_CYCLES_L2, LATENCY_CYCLES_MEM);

            } else {
                latency_counter = LATENCY_CYCLES_L2;
                if (PRINT && client_pending == FETCH)
                    fprintf(fout1, "Acierto en cache, esperando instruccion a cache (%d clk)", LATENCY_CYCLES_L2);
                else if (PRINT && client_pending == DATAMEM_R)
                    fprintf(fout1, "Acierto en cache, esperando dato a cache (%d clk)", LATENCY_CYCLES_L2);
            }

            pending_response = true;
        }else if (client_pending == DATAMEM_W) {
            // --- BLOQUE DE ESCRITURA ---
            dataCacheLine lineL1 = dataMem_data.read();

            sc_uint<32> index = (addr_buf >> 2) / WORDSPERLINE_L2 % NUMLINES_L2;
            sc_uint<32> tag = addr_buf >> (2 + int(log2(WORDSPERLINE_L2)) + int(log2(NUMLINES_L2)));
            unsigned l2_offset = ((addr_buf >> 2) % WORDSPERLINE_L2) / WORDSPERLINE_L1_D;
            unsigned l2_base = l2_offset * WORDSPERLINE_L1_D;

            L2CacheSet &set = cache[index];

            bool hit = false;
            for (auto &line: set.ways) {
                if (line.valid && line.tag == tag) {
                    // Copiar la línea de L1 en la posición correspondiente de L2
                    for (unsigned i = 0; i < WORDSPERLINE_L1_D; ++i) {
                        if ((l2_base + i) < WORDSPERLINE_L2)
                            line.data[l2_base + i] = lineL1.data[i];
                    }
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
                    // Si corresponde al rango de la línea L1, copia el dato de L1, si no, lee de memoria
                    if (i >= l2_base && i < l2_base + WORDSPERLINE_L1_D)
                        newline.data[i] = lineL1.data[i - l2_base];
                    else {
                        sc_uint<32> currAddr = (addr_buf & ~(WORDSPERLINE_L2 * 4 - 1)) + i * 4;
                        sc_int<32> mem_word = MEM->readWord(currAddr);
                        newline.data[i] = mem_word;
                    }
                }

                writeLine(addr_buf, newline);
                latency_counter = LATENCY_CYCLES_L2 + LATENCY_CYCLES_MEM;
                if (PRINT)
                    fprintf(fout1, "Fallo en cache, esperando dato a cache y escritura a mem (%d clk) + mem (%d clk)", LATENCY_CYCLES_L2, LATENCY_CYCLES_MEM);
            } else {
                latency_counter = LATENCY_CYCLES_L2;
                if (PRINT) fprintf(fout1, "Acierto en cache, esperando escritura a mem (%d clk)", LATENCY_CYCLES_L2);
            }

            // Escribir toda la línea L1 en memoria principal
            for (unsigned i = 0; i < WORDSPERLINE_L1_D; ++i) {
                if (MEM->isValidAddress(addr_buf + i * 4))
                    MEM->writeWord(addr_buf + i * 4, lineL1.data[i]);
            }
            pending_response = true;
        }
    }

    notifying_to_dataMem = false;
    notifying_to_fetch = false;

    // --- RESPUESTA ---
    if (pending_response && --latency_counter <= 0) {
        if (client_pending == FETCH) {
            fetch_line_out.write(buffer_out);
            fetch_ready.write(true);
            notifying_to_fetch = true;
            if (PRINT) fprintf(fout1, "Instruccion almacenada en cache");
        } else if (client_pending == DATAMEM_R) {
            dataMem_line_out.write(buffer_out);
            read_ready.write(true);
            notifying_to_dataMem = true;
            if (PRINT) fprintf(fout1, "Dato almacenado en cache");
        } else if (client_pending == DATAMEM_W) {
            dataMem_line_out.write(buffer_out);
            write_ready.write(true);
            notifying_to_dataMem = true;
            if (PRINT) fprintf(fout1, "Dato guardado en memoria");
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
                sc_int<32> word = victim.data[i];
                MEM->writeWord(base_addr + 4 * i, word);
            }
        }

        // Eliminar línea víctima
        set.ways.erase(it);
    }

    // Insertar nueva línea
    set.ways.push_back(newline);
    sharedFlag = 0;
}

void cacheL2::printCacheL2() {
    // Cabecera CSV
    fprintf(fout6, "%.0f\nIndex;Way;Valid;Dirty;Tag", tiempo);
    for (unsigned i = 0; i < WORDSPERLINE_L2; ++i) {
        fprintf(fout6, ";Data%u", i);
    }
    fprintf(fout6, "\n");

    for (unsigned index = 0; index < NUMLINES_L2; ++index) {
        const L2CacheSet &set = cache[index];
        for (unsigned way = 0; way < ASSOCIATIVITY_L2; ++way) {
            if (way < set.ways.size()) {
                const L2CacheLine &line = set.ways[way];
                fprintf(fout6, "%u;%u;%u;%u;0x%08X",
                        index,
                        way,
                        line.valid ? 1 : 0,
                        line.dirty ? 1 : 0,
                        (unsigned int) line.tag);
                for (unsigned i = 0; i < WORDSPERLINE_L2; ++i) {
                    fprintf(fout6, ";0x%08X", (unsigned int) line.data[i]);
                }
            } else {
                // Línea/vía no inicializada: imprime valores por defecto
                fprintf(fout6, "%u;%u;0;0;0x00000000", index, way);
                for (unsigned i = 0; i < WORDSPERLINE_L2; ++i) {
                    fprintf(fout6, ";0xDEADBEEF");
                }
            }
            fprintf(fout6, "\n");
        }
    }
    sharedFlag++;
}

void cacheL2::updateLRU(L2CacheSet &set, L2CacheLine &accessedLine) {
    if (!USEFIFO_L2) {
        accessedLine.lru_counter = current_lru++;
    }
}
