#ifndef PARAMS_H
#define PARAMS_H

// Parámetros de configuración de la caché L1 de instrucciones
#define NUMLINES_L1_I 16
#define WORDSPERLINE_L1_I 4
#define ASSOCIATIVITY_L1_I 1	// 1 = directa, 2 = 2 vías, X = X vías
#define USEFIFO_L1_I false

// Parámetros de configuración de la caché L1 de datos
#define WORDSPERLINE_L1_D 2
#define NUMLINES_L1_D 16
#define ASSOCIATIVITY_L1_D 1
#define USEFIFO_L1_D true
#define USEWBACK_L1_D false		// false = write-through, true = write-back
#define MAX_SIZE_QUEUE 2		// El tamaño mínimo de la cola debe de ser 2 para poder manejar el pipeline

// Parámetros de configuración de la caché L2
#define NUMLINES_L2 32
#define WORDSPERLINE_L2 8
#define ASSOCIATIVITY_L2 1
#define USEFIFO_L2 true
#define LATENCY_CYCLES_L2 2
#define LATENCY_CYCLES_MEM 3

#endif