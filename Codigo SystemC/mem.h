#ifndef MEM_H
#define MEM_H

#include "structsRV.h"
#include "systemc.h"

// memoria unificada, por necesidades de la carga de archivos ELF
// el acceso sigue siendo tipo Harvard


class mem {

public:
    void addMemSeg(unsigned int init, unsigned int length);
    void writeByte(int addr, int data);
    void writeWord(int addr, int data);
    int readWord(int addr);

    mem() {
        MEM = 0;// empezamos sin ningún segmento de memoria
    }


    ~mem() {
        struct memSeg *next;

        while (MEM) {
            next = MEM->next;
            delete MEM->mem;
            delete MEM;
            MEM = next;
        };
    }

private:
    struct memSeg *MEM;
};


#endif