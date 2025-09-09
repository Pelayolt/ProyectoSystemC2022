#include "mem.h"

void mem::addMemSeg(unsigned int init, unsigned int length)
{
	struct memSeg* pt;

	if (!MEM) {
		pt = MEM = new struct memSeg;
	}
	else {
		pt = MEM;
		while (pt->next)
			pt = pt->next;
		pt->next = new struct memSeg;
		pt = pt->next;
	}
	if (!pt) {
		cerr << "ERROR, no se puede crear banco de memoria" << endl;
		exit(-1);
	}

	pt->init = init;
	pt->end = init + length - 1;
	pt->length = length;
	pt->mem = new unsigned int[length >> 2];
	if (!pt->mem) {
		cerr << "ERROR, no se puede reserver memoria para el banco de longitud " << length << endl;
		exit(-1);
	}
	pt->next = 0;

}


void mem::writeWord(int addr, int data) {

	struct memSeg* pt;

	pt = MEM;
	while (1) {
		if ((addr >= pt->init) && (addr <= pt->end)) {
			pt->mem[(addr - (pt->init)) >> 2] = data;
			return;
		}
		pt = pt->next;
		if (!pt) {
			cerr << "ERROR. Direccion de memoria " << hex << addr << " no accesible" << endl;
			exit(-1);
		}
	}

}

int mem::readWord(int addr) {

	struct memSeg* pt;

	pt = MEM;
	while (1) {
		if ((addr >= pt->init) && (addr <= pt->end)) {
			return pt->mem[(addr - (pt->init)) >> 2];
		}
		pt = pt->next;
		if (!pt) {
			cerr << "ERROR. Direccion de memoria " << hex << addr << " no accesible" << endl;
			exit(-1);
		}
	}

}


void mem::writeByte(int addr, int data) {

	struct memSeg* pt;
	unsigned char* loc;

	pt = MEM;
	while (1) {
		if ((addr >= pt->init) && (addr <= pt->end)) {
			loc = (unsigned char*)(pt->mem);
			loc[addr - (pt->init)] = (unsigned char)data;
			return;
		}
		pt = pt->next;
		if (!pt) {
			cerr << "ERROR. Direccion de memoria " << hex << addr << " no accesible" << endl;
			exit(-1);
		}
	}

}

bool mem::isValidAddress(int addr) {
    memSeg *pt = MEM;
    while (pt) {
        if (addr >= pt->init && addr <= pt->end)
            return true;
        pt = pt->next;
    }
    return false;
}