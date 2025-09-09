#ifndef ALU_H
#define ALU_H

#include "systemc.h"
#include "structsRV.h"


SC_MODULE (alu) {
public:

sc_in<bool> clk, rst;

sc_in< instruction >	I; 
sc_out < instruction >	instOut;

  void registro();

  SC_CTOR(alu) {
	cout<<"alu: "<<name()<<endl;

	INST.address = 0xffffffff;	INST.I = 0x13;	INST.aluOp = 0; INST.memOp = 15;
	INST.rs1 = INST.rs2 = INST.rd = 0x1f; // 
	INST.wReg = false; 
	INST.opA = INST.opB = INST.val2 = INST.aluOut = INST.dataOut = 0x0000dead;
	strcpy(INST.desc, "???");

	SC_METHOD(registro);
	sensitive << clk.pos(); 

  }
private:

	instruction INST;

}; 


#define ADD 0
#define SLL 1
#define SLT 2
#define SLTU 3
#define XOR 4
#define SRL 5
#define OR 6
#define AND 7
#define SUB 8
#define SRA 13

/*
#define MUL 16
#define MULH 17
#define MULHSU 18
#define MULHU 19
#define DIV 20
#define DIVU 21
#define REM 22
#define REMU 23
*/

#endif