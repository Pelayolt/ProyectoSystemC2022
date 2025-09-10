#include "systemc.h"
#include <stdio.h>
#include <time.h>

#include "coreRiscV.h"


int sc_main(int nargs, char *vargs[]) {

    sc_clock clk("clk", 1);// ciclo de 1 ns
    sc_signal<bool> rst;
    FILE *elf;
    time_t begin, end;

    if (nargs != 2) {
        cerr << "ERROR. Se debe especificar el archivo ELF" << endl;
        exit(-1);
    }

    elf = fopen(vargs[1], "rb");
    if (!elf) {
        cerr << "ERROR. No puedo abrir el archivo " << vargs[1] << endl;
        exit(-1);
    }

    coreRiscV instCoreRiscV("core");

    instCoreRiscV.clk(clk);
    instCoreRiscV.rst(rst);

    if (instCoreRiscV.leeELF(elf)) {
        fclose(elf);
        cerr << "ERROR leyendo archivo " << vargs[1] << endl;
        exit(-1);
    }

    rst.write(true);
    sc_start(2, SC_NS);
    time(&begin);
    rst.write(false);
    sc_start(10, SC_SEC);
    time(&end);

    printf("Tiempo %ld\n", end - begin);

    return 0;
}
