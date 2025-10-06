#ifndef PARAMS_H
#define PARAMS_H

// Parámetros de configuración de la caché L1 de instrucciones
#define ASSOCIATIVITY_L1_I 4// 1 = directa, 2 = 2 vías, X = X vías
#define NUMLINES_L1_I 4
#define WORDSPERLINE_L1_I 4
#define USEFIFO_L1_I true

// Parámetros de configuración de la caché L1 de datos
#define ASSOCIATIVITY_L1_D 4
#define NUMLINES_L1_D 4
#define WORDSPERLINE_L1_D 4
#define USEFIFO_L1_D false
#define USEWBACK_L1_D false		// false = write-through, true = write-back
#define MAX_QUEUE_SIZE 4  // El tamaño mínimo de la cola debe de ser 2 para poder manejar el pipeline

// Parámetros de configuración de la caché L2
#define ASSOCIATIVITY_L2 8
#define NUMLINES_L2 4
#define WORDSPERLINE_L2 8
#define USEFIFO_L2 true
#define USEWBACK_L2 true		// false = write-through, true = write-back
#define LATENCY_CYCLES_L2 10
#define LATENCY_CYCLES_MEM 100

#endif