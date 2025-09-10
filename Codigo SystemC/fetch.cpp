#include "fetch.h"


void fetch::updatePC() {

    sc_uint<32> tmp;

    tmp = PCext.read();

    tiempo = sc_time_stamp().to_double() / 1000.0;

    if (bubble.read())
        newPC = PCext.read();
    else if (hazard.read())
        newPC = PC;
    else
        newPC = PC + 4;
}

void fetch::registro() {

    sc_uint<32> tmp;

    tmp = PCext.read();

    if (rst.read()) {
        PC = 0;
        INST.address = PC;
        INST.I = 0x00000013;// nop = addi x0, x0, 0
        PCout.write(PC);
        numInst = 0;
    } else {

        if (bubble.read()) {
            INST.I = 0x00000013;// nop = addi x0, x0, 0
            PCout.write(PC);    // NO ESTOY SEGURO
        } else {
            if (hazard.read()) {
                // no hace nada, se mantienen las salidas del ciclo anterior
            } else {
                INST.I = MEM->readWord(PC);
                INST.address = PC;
                PCout.write(PC);
                ++numInst;
            }
        }
        PC = newPC;
    }

    instOut.write(INST);
    fire.write(!fire.read());
}
