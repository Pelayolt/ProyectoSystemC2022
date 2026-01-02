#include "systemc.h"
#include "params.h"
#include <stdio.h>
#include <time.h>

#include "coreRiscV.h"
#include <direct.h>
/*
BENCHMARKS ELF FILES:

-- Prueba de conflictos y asociatividad (para L1-D) --
..\Codigos_Segger\qsort\Output\Debug\Exe\qsortN.elf

-- Mejor caso de localidad espacial (para L1-D) --
..\Codigos_Segger\multiply\Output\Debug\Exe\multiply.elf

-- Peor caso de localidad espacial (para L1-D) --
..\Codigos_Segger\spmv\Output\Debug\Exe\spmv.elf

-- Mejor caso de localidad espacial (para L1-I) --
..\Codigos_Segger\towers\Output\Debug\Exe\towers.elf
*/

FILE *fout1, *fout2, *fout3, *fout4, *fout5, *fout6, *fout7;
std::string bench_mark_name;

void openFiles() {
    _mkdir("..\\Output");

    fout1 = fopen("..\\Output\\Output_pipeline.txt", "w");
    if (!fout1) {
        perror("No se pudo abrir Output_pipeline.txt");
        exit(1);
    }
    fout2 = fopen("..\\Output\\Output_comparar_datos.txt", "w");
    if (!fout2) {
        perror("No se pudo abrir Output_comparar_datos.txt");
        exit(1);
    }
    fout3 = fopen("..\\Output\\Output_comparar_instrucciones.txt", "w");
    if (!fout3) {
        perror("No se pudo abrir Output_comparar_instrucciones.txt");
        exit(1);
    }
    fout4 = fopen("..\\Output\\Output_cache_datos.txt", "w");
    if (!fout4) {
        perror("No se pudo abrir Output_cache_datos.txt");
        exit(1);
    }
    fout5 = fopen("..\\Output\\Output_cache_instrucciones.txt", "w");
    if (!fout5) {
        perror("No se pudo abrir Output_cache_instrucciones.txt");
        exit(1);
    }
    fout6 = fopen("..\\Output\\Output_cache_L2.txt", "w");
    if (!fout6) {
        perror("No se pudo abrir Output_cache_L2.txt");
        exit(1);
    }
    fout7 = fopen("..\\Output\\Output_stats.txt", "w");
    if (!fout7) {
        perror("No se pudo abrir Output_stats.txt");
        exit(1);
    }
}

static std::string obtenerNombreSinExtension(const std::string rutaCompleta) {

    size_t posBarra = rutaCompleta.find_last_of("/\\");
    size_t inicioNombre;
    if (posBarra == std::string::npos) {
        inicioNombre = 0;
    } else {
        inicioNombre = posBarra + 1;
    }

    size_t posPunto = rutaCompleta.find_last_of('.');

    size_t longitud;
    if (posPunto == std::string::npos || posPunto < inicioNombre) {
        longitud = std::string::npos;
    } else {
        longitud = posPunto - inicioNombre;
    }

    return rutaCompleta.substr(inicioNombre, longitud);
}

int sc_main(int nargs, char *vargs[]) {

	sc_clock clk("clk", 1); // ciclo de 1 ns
	sc_signal <bool> rst;
	FILE* elf; 
	time_t begin, end;	


	openFiles();
	
	if (nargs != 2) {
		cerr << "ERROR. Se debe especificar el archivo ELF" << endl;
		exit(-1);
	}
     
    bench_mark_name = obtenerNombreSinExtension(vargs[1]);

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
    fclose(fout3);
    fclose(fout4);
    fclose(fout5);
    fclose(fout6);
    fclose(fout7);

	return 0;
	   
}


