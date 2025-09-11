#ifndef STRUCTSRV_H
#define STRUCTSRV_H

#include "systemc.h"
#include <string>
#define WORDSPERLINE_L2 4
#define PRINT true

using namespace std;


struct f_header {
	unsigned int magic;
	unsigned char ei_class, ei_data, ei_version, ei_osabi, ei_abiversion, ei_pad0;
	unsigned short ei_pad12, ei_pad34, ei_pad56;
	unsigned short e_type, e_machine;
	unsigned int e_version, e_entry, e_phoff, e_shoff, e_flags;
	unsigned short e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
};

struct p_header {
	unsigned int p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align;
};

struct s_header {
	unsigned int sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link, sh_info, sh_addralign, sh_entsize;
};


struct instruction{
public:
	sc_uint<32> address, I; 
	sc_uint<5> rs1, rs2, rd; 
	sc_uint<5> aluOp;
	sc_uint<4> memOp;
	bool wReg; 
	sc_int<32> opA, opB, val2, aluOut, dataOut;
	char desc[8]; 
	inline bool operator ==(const instruction& inst) const {
		return ( (inst.address == address) && (inst.I == I) && (inst.wReg == wReg) && (!strcmp(inst.desc, desc)) );
	}
};

inline ostream& operator <<(ostream& os, const instruction& inst) {
	os << "Inst: " << hex << inst.I << "@" << inst.address << " " << inst.desc;
	return os;
}

inline void sc_trace(sc_trace_file* tf, const instruction& inst, const std::string& name) {
    sc_trace(tf, inst.I, name + ".I");

}

struct memSeg;

struct memSeg {
	unsigned int init, end, length; // en bytes
	unsigned int* mem;
	struct memSeg* next;
};

struct instCacheLine {
    bool valid;
    sc_uint<32> tag;
    std::vector<sc_uint<32>> data;
    unsigned int lru_counter = 0;
};

struct dataCacheLine {
    bool valid;
    bool dirty = false;
    sc_uint<32> tag;
    std::vector<sc_uint<32>> data;
    unsigned int lru_counter = 0;
};

struct L2CacheLine {
public:
    bool valid;
    bool dirty = false;
    sc_uint<32> tag;
    sc_biguint<32 * WORDSPERLINE_L2> data;
    unsigned int lru_counter = 0;
    bool operator==(const L2CacheLine &other) const {
        return valid == other.valid && tag == other.tag && data == other.data && lru_counter == other.lru_counter;
    }
};

inline void sc_trace(sc_trace_file *tf, const L2CacheLine &line, const std::string &name) {
    sc_trace(tf, line.valid, name + ".valid");
    sc_trace(tf, line.dirty, name + ".dirty");
    sc_trace(tf, line.tag, name + ".tag");
    sc_trace(tf, line.data, name + ".data");
    sc_trace(tf, line.lru_counter, name + ".lru");
}

inline std::ostream &operator<<(std::ostream &os, const L2CacheLine &line) {
    os << "{ valid: " << line.valid << ", dirty: " << line.dirty << ", tag: 0x" << std::hex << line.tag << ", data: 0x" << std::hex << line.data << ", lru: " << std::dec << line.lru_counter << " }";
    return os;
}
#endif
