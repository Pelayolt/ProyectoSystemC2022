#include "dataMem.h"


void dataMem::registro() {

    sc_int<32> address, dataWrite, UW;
    sc_int<32> SW;
    sc_int<16> half;
    sc_int<8> byte;
    sc_uint<4> opCode;
    int BH;
    double tiempo;

    tiempo = sc_time_stamp().to_double() / 1000.0;// podemos saber en qué ciclo estamos con este truco

    if (rst.read()) {
        instOut.write(INST);
    } else {
        INST = I.read();

        ////////////		cout << INST << endl;	se puede imprimir la instrucción en casi cualquier parte
        ////////////		esto ayuda a depurar el código

        address = INST.aluOut;
        BH = address & 3;// byte or half-word
        dataWrite = INST.val2;
        opCode = INST.memOp;
        if (opCode < 15) {
            SW = MEM->readWord(address);
            UW = (sc_uint<32>) SW;
        }

        // COMPROBAR que se hacen bien las conversiones signed/unsigned

        switch (opCode) {
            case 0:
                byte = SW(8 * BH + 7, 8 * BH);// lee un byte
                INST.dataOut = byte;
                break;
            case 1:
                if (BH & 1)
                    cerr << "Acceso de lectura de media palabra desalineado @" << address << endl;
                half = SW(8 * BH + 15, 8 * BH);// lee media palabra
                INST.dataOut = half;
                break;
            case 2:
                if (BH & 3)
                    cerr << "Acceso de lectura de palabra desalineado @" << address << endl;
                //	DEBUG			printf("[%06x] -> %08x   @ %.0lf \n", address.to_int(), SW.to_int(), sc_time_stamp().to_double() / 1000.0);
                INST.dataOut = SW;
                ;
                break;// lee una palabra

            case 4:
                byte = SW(8 * BH + 7, 8 * BH);// lee un byte unsigned
                INST.dataOut = (sc_int<32>) ((sc_uint<8>) byte);
                //  DEBUG			cout << hex << "lbu " << (sc_int<32>)((sc_uint<8>)byte) << " @" << address << " -> " << INST.rd << "  " << INST.address << endl;
                break;
            case 5:
                if (BH & 1)
                    cerr << "Acceso de lectura de media palabra desalineado @" << address << endl;
                half = SW(8 * BH + 15, 8 * BH);// lee media palabra unsigned
                INST.dataOut = (sc_int<32>) ((sc_uint<16>) half);
                break;

            case 8:
                SW(8 * BH + 7, 8 * BH) = dataWrite(7, 0);// escribe un byte
                MEM->writeWord(address, SW);
                break;
            case 9:
                if (BH & 1)
                    cerr << "Acceso de escritura de media palabra desalineado @" << address << endl;
                SW(8 * BH + 15, 8 * BH) = dataWrite(15, 0);// escribe media palabra
                MEM->writeWord(address, SW);
                break;
            case 10:
                if (BH & 3)// escribe una palabra
                    cerr << "Acceso escritura lectura de palabra desalineado @" << address << endl;

                // DEBUG			printf("[%06x] <- %08x   @ %.0lf \n", address.to_int(), dataWrite.to_int(), sc_time_stamp().to_double() / 1000.0);
                MEM->writeWord(address, dataWrite);
                break;
            case 15:
                INST.dataOut = INST.aluOut;
                break;
            default:
                cerr << "Error at the dataMem, unknown memOpCode " << opCode << endl;
                exit(-1);
        };


        instOut.write(INST);
    }
}
