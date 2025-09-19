#include"dataMem.h"
#include <iomanip>

extern FILE *fout1, *fout2, *fout4;

void dataMem::initCache() {
    cache.resize(NUMLINES_L1_D);
    for (auto &set: cache) {
        set.ways.resize(ASSOCIATIVITY_L1_D);
        for (auto &line: set.ways) {
            line.valid = false;
            line.dirty = false;
            line.tag = 0;
            for (unsigned i = 0; i < WORDSPERLINE_L1_D; i++) {
                line.data[i] = 0xDEADBEEF;
            }
            line.lru_counter = 0;
        }
    }
}

void dataMem::updatePendingMask() {
    std::bitset<32> mask;
    mask.reset();

    std::queue<instruction> tmp = pendingQueue;// copia temporal
    while (!tmp.empty()) {
        instruction p = tmp.front();
        if (p.wReg && p.rd != 0)
            mask.set(p.rd);
        tmp.pop();
    }
    pendingRdMask.write(mask.to_ulong());
    queueAvailableSpace.write(MAX_SIZE_QUEUE - pendingQueue.size());
}

bool dataMem::accessCache(sc_int<32> addr, sc_int<32> &word, bool isWrite, sc_int<32> writeData) {
    sc_uint<32> index = (addr >> 2) / WORDSPERLINE_L1_D % NUMLINES_L1_D;
    sc_uint<32> tag = addr >> (2 + int(log2(WORDSPERLINE_L1_D)) + int(log2(NUMLINES_L1_D)));
    sc_uint<32> offset = (addr >> 2) % WORDSPERLINE_L1_D;

    CacheSet &set = cache[index];
    for (auto &line: set.ways) {
        if (line.valid && line.tag == tag) {
            if (isWrite) {
                line.data[offset] = writeData;

                if (USEWBACK_L1_D) {
                    // WRITE-BACK: marcar línea como sucia
                    line.dirty = true;
                } else {
                    // WRITE-THROUGH: escribir en L2 en el siguiente ciclo
                    pendingWriteL2 = true;
                    pending_addr = addr;
                    pending_line = line; 
                }
                sharedFlag = 0;
            } else {
                word = line.data[offset];
            }
            line.lru_counter = current_lru++;
            return true;
        }
    }

    return false;// MISS
}

void dataMem::storeLineToL1(sc_uint<32> addr, const L2CacheLine &lineL2) {
    sc_uint<32> index = (addr >> 2) / WORDSPERLINE_L1_D % NUMLINES_L1_D;
    sc_uint<32> tag = addr >> (2 + int(log2(WORDSPERLINE_L1_D)) + int(log2(NUMLINES_L1_D)));
    unsigned offset = ((addr >> 2) & (WORDSPERLINE_L2 - 1)) / WORDSPERLINE_L1_D;
    unsigned l2_base = offset * WORDSPERLINE_L1_D;

    dataCacheLine newline;
    newline.valid = true;
    newline.tag = tag;
    newline.lru_counter = current_lru++;
    newline.dirty = false;      // recién cargada desde L2 -> no sucia todavía

    // Copiar desde L2 a la nueva línea, comprobando rango
    for (unsigned i = 0; i < WORDSPERLINE_L1_D; ++i) {
        unsigned l2_idx = l2_base + i;
        if (l2_idx < WORDSPERLINE_L2)
            newline.data[i] = lineL2.data[l2_idx];
        else
            newline.data[i] = 0xDEADBEEF; // Relleno si el índice se sale del rango
    }

    CacheSet &set = cache[index];
    if (set.ways.size() >= ASSOCIATIVITY_L1_D) {
        // Buscar víctima según FIFO o LRU
        auto victim_it = USEFIFO_L1_D ? set.ways.begin() : std::min_element(set.ways.begin(), set.ways.end(), [](const dataCacheLine &a, const dataCacheLine &b) {
            return a.lru_counter < b.lru_counter;
        });

        // Si se descarta una línea sucia en modo Write-Back -> programar escritura en L2
        if (USEWBACK_L1_D && victim_it->dirty) {
            pendingWriteL2 = true;
            pending_addr = reconstructAddress(victim_it->tag, index);
            pending_line = *victim_it;
        }

        set.ways.erase(victim_it);
    }
    set.ways.push_back(newline);
    sharedFlag = 0;
}

void dataMem::startL2RequestR(sc_int<32> addr) {
    addr_buf = addr;
    addr_cacheL2.write(addr & ~(WORDSPERLINE_L2 * 4 - 1));
    read_req_cacheL2.write(true);
    waitingL2 = true;
}

void dataMem::startL2RequestW(sc_int<32> addr, const dataCacheLine &writeLine) {
    addr_cacheL2.write((sc_uint<32>) addr);
    line_out.write(writeLine);
    write_req_cacheL2.write(true);
    waitingL2 = true;
}

bool dataMem::isL2RequestCompleteR() {
    if (read_ready_cacheL2.read()) {
        read_req_cacheL2.write(false);
        storeLineToL1(addr_buf, line_in.read());
        waitingL2 = false;
        if (PRINT) fprintf(fout1, "Dato recibido y almacenado en cache");
        
        return true;
    }
    return false;
}

bool dataMem::isL2RequestCompleteW() {
    if (write_ready_cacheL2.read()) {
        write_req_cacheL2.write(false);
        waitingL2 = false;
        if (PRINT) fprintf(fout1, "Dato escrito en cacheL2");
        
        return true;
    }
    return false;
}

void dataMem::registro() {

    tiempo = sc_time_stamp().to_double() / 1000.0;

    if (rst.read()) {
        instOut.write(INST);
        waitingL2 = false;
        while (!pendingQueue.empty()) pendingQueue.pop();
        updatePendingMask();
        sharedFlag = 3;
        return;
    }
    if (PRINT && sharedFlag < 3) printCacheL1Data();

    instruction nextInst = I.read();
    bool serveQueue = !pendingQueue.empty() && (nextInst.memOp < 15 || !nextInst.wReg || nextInst.I == 0x00000013);
    
    if (nextInst.memOp < 15) {
        pendingQueue.push(nextInst);
        updatePendingMask();
    }

    if (serveQueue)
        INST = pendingQueue.front();
    else
        INST = nextInst;

    if (PRINT) {
        fprintf(fout1, ";CACHE DATOS;0x%08X;", static_cast<unsigned int>(INST.I));
        printPendings();
    }

    sc_int<32> address = INST.aluOut;
    int BH = address & 3;
    sc_int<32> dataWrite = INST.val2;
    sc_uint<4> opCode = INST.memOp;
    sc_int<32> word;

    if (tiempo >= 188420)
        cout << "";

    if (pendingWriteL2 && !waitingL2) {
        startL2RequestW(pending_addr, pending_line);
        pendingWriteL2 = false;
    } 

    if (opCode < 15 && !accessCache(address, word, false, 0)) {
        // MISS -> encolamos y lanzamos refill si hace falta
        if (!waitingL2) {
            startL2RequestR(address);
            if (PRINT) fprintf(fout1, "Fallo en cache, solicitando dato a cacheL2");

        } else if (isL2RequestCompleteR() || isL2RequestCompleteW()) {
        } else if (PRINT) {
            fprintf(fout1, "Esperando a que cacheL2 procese la solicitud");
        }

        INST.wReg = false;
        instOut.write(INST);
        return;
    } else if (opCode < 15) {
        // HIT -> Sacamos de la cola y procesamos el load o store
        pendingQueue.pop();
        updatePendingMask();
    }

    if (waitingL2 && (isL2RequestCompleteR() || isL2RequestCompleteW())) {
        if (PRINT) fprintf(fout1, " y ");
    }
    
    switch (opCode) {
        // ----- Lecturas -----
        case 0:
        case 1:
        case 2:
        case 4:
        case 5:
            if ((opCode == 1 || opCode == 5) && (BH & 1))
                cerr << "Lectura desalineada @" << address << endl;
            if (opCode == 2 && (BH & 3))
                cerr << "Lectura palabra desalineada @" << address << endl;
        
            INST.dataOut = decodeReadData(opCode, word, BH);
            instOut.write(INST);
            if (PRINT) {
                fprintf(fout1, "Palabra 0x%08X encontrada en cache", static_cast<unsigned int>(INST.dataOut));
                fprintf(fout2, "%.0f;LOAD;0x%08X;R%u;0x%08X;0x%08X\n", tiempo, static_cast<unsigned int>(address), static_cast<unsigned int>(INST.rd.to_uint()), static_cast<unsigned int>(INST.dataOut), static_cast<unsigned int>(MEM->readWord(address)));
            }
            break;

        // ----- Escrituras -----
        case 8:// SB
            word.range(8 * BH + 7, 8 * BH) = dataWrite.range(7, 0);
            accessCache(address, word, true, word);
            instOut.write(INST);
            if (PRINT) {
                fprintf(fout1, "Palabra 0x%08X escrita en cache", static_cast<unsigned int>(word));
                fprintf(fout2, "%.0f;STORE;0x%08X;R%u;0x%08X;\n", tiempo, static_cast<unsigned int>(address), static_cast<unsigned int>(INST.rd.to_uint()), static_cast<unsigned int>(word)); 
            }
                break;

        case 9:// SH
            if (BH & 1) cerr << "Escritura desalineada @" << address << endl;
            word.range(8 * BH + 15, 8 * BH) = dataWrite.range(15, 0);
            accessCache(address, word, true, word);
            instOut.write(INST);
            if (PRINT) {
                fprintf(fout1, "Palabra 0x%08X escrita en cache", static_cast<unsigned int>(word));
                fprintf(fout2, "%.0f;STORE;0x%08X;R%u;0x%08X;\n", tiempo, static_cast<unsigned int>(address), static_cast<unsigned int>(INST.rd.to_uint()), static_cast<unsigned int>(word));
            }
            break;

        case 10:// SW
            if (BH & 3) cerr << "Escritura palabra desalineada @" << address << endl;
            accessCache(address, word, true, dataWrite);
            instOut.write(INST);
            if (PRINT) {
                fprintf(fout1, "Palabra 0x%08X escrita en cache", static_cast<unsigned int>(dataWrite));
                fprintf(fout2, "%.0f;STORE;0x%08X;R%u;0x%08X;\n", tiempo, static_cast<unsigned int>(address), static_cast<unsigned int>(INST.rd.to_uint()), static_cast<unsigned int>(dataWrite));
            }
            break;

        case 15:// Passthrough
            INST.dataOut = INST.aluOut;
            instOut.write(INST);
            if (PRINT) fprintf(fout1, "No usa cache");

            break;

        default:
            cerr << "Operación de memoria desconocida: " << opCode << endl;
            exit(-1);
    }    
}

sc_int<32> dataMem::decodeReadData(sc_uint<4> op, sc_int<32> word, int BH) {
    switch (op) {
        case 0:// LB
            return  (sc_int<8>) ((word >> (8 * BH)) & 0xFF);
        case 1:// LH
            return (sc_int<16>) ((word >> (8 * BH)) & 0xFFFF);
        case 2:// LW
            return (sc_int<32>) word;
        case 4:// LBU
            return (sc_int<32>) (sc_uint<8>) ((word >> (8 * BH)) & 0xFF);
        case 5:// LHU
            return (sc_int<32>) (sc_uint<16>) ((word >> (8 * BH)) & 0xFFFF);
        default:
            return 0;
    }
}

sc_uint<32> dataMem::reconstructAddress(sc_uint<32> tag, sc_uint<32> index) {
    // dirección base de la línea víctima
    return (tag << (int(log2(WORDSPERLINE_L1_D)) + int(log2(NUMLINES_L1_D)))) | (index * WORDSPERLINE_L1_D * 4);
}

void dataMem::printPendings() {
    std::queue<instruction> tmp = pendingQueue;// copia temporal
    fprintf(fout1, "Pendientes;");

    while (!tmp.empty()) {
        instruction p = tmp.front();
        tmp.pop();
        if (!tmp.empty())
            fprintf(fout1, "0x%08X | ", static_cast<unsigned int>(p.I));

        else
            fprintf(fout1, "0x%08X", static_cast<unsigned int>(p.I));
    }
    fprintf(fout1, ";");
}

void dataMem::printCacheL1Data() {
    // Cabecera CSV
    fprintf(fout4, "%.0f\nIndex;Way;Valid;Dirty;Tag", tiempo);
    for (unsigned i = 0; i < WORDSPERLINE_L1_D; ++i) {
        fprintf(fout4, ";Data%u", i);
    }
    fprintf(fout4, "\n");

    for (unsigned index = 0; index < NUMLINES_L1_D; ++index) {
        const CacheSet &set = cache[index];
        for (unsigned way = 0; way < ASSOCIATIVITY_L1_D; ++way) {
            // Si la vía existe, imprime su contenido; si no, imprime valores por defecto
            if (way < set.ways.size()) {
                const dataCacheLine &line = set.ways[way];
                fprintf(fout4, "%u;%u;%u;%u;0x%08X",
                    index,
                    way,
                    line.valid ? 1 : 0,
                    line.dirty ? 1 : 0,
                    (unsigned int)line.tag);
                for (unsigned i = 0; i < WORDSPERLINE_L1_D; ++i) {
                    fprintf(fout4, ";0x%08X", (unsigned int)line.data[i]);
                }
            } else {
                // Línea/vía no inicializada: imprime valores por defecto
                fprintf(fout4, "%u;%u;0;0;0x00000000", index, way);
                for (unsigned i = 0; i < WORDSPERLINE_L1_D; ++i) {
                    fprintf(fout4, ";0xDEADBEEF");
                }
            }
            fprintf(fout4, "\n");
        }
    }
    sharedFlag++;
}