#include "systemc.h"
#include <stdio.h>
#include <time.h>

#include "coreRiscV.h"

FILE *fout1, *fout2, *fout4, *fout5, *fout6;

int sc_main(int nargs, char *vargs[]) {

	sc_clock clk("clk", 1); // ciclo de 1 ns
	sc_signal <bool> rst;
	FILE* elf; 
	time_t begin, end;	

	fout1 = fopen("..\\Excels\\Output_pipeline.txt", "w");
    if (!fout1) {
        perror("No se pudo abrir Output_pipeline.txt");
        return 1;
    }

	fout2 = fopen("..\\Excels\\Output_comparar.txt", "w");
    if (!fout2) {
        perror("No se pudo abrir Output_comparar.txt");
        return 1;
    }
	fout4 = fopen("..\\Excels\\Output_cache_datos.txt", "w");
    if (!fout4) {
        perror("No se pudo abrir Output_cache_datos.txt");
        return 1;
    }

	fout5 = fopen("..\\Excels\\Output_cache_instrucciones.txt", "w");
    if (!fout5) {
        perror("No se pudo abrir Output_cache_instrucciones.txt");
        return 1;
    }
	fout6 = fopen("..\\Excels\\Output_cache_L2.txt", "w");
    if (!fout6) {
        perror("No se pudo abrir Output_cache_L2.txt");
        return 1;
    }
	
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

	rst.write(true); sc_start(2, SC_NS);
	time(&begin);
	rst.write(false); sc_start(10, SC_SEC);
	time(&end);

	

	printf("Tiempo %ld\n", end - begin);

    fclose(fout1);
    fclose(fout2);
    fclose(fout4);
    fclose(fout5);
    fclose(fout6);

	return 0;
	   
}


