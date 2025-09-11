#ifndef PARAMS_H
#define PARAMS_H

// Par�metros de configuraci�n de la cach� L1 de instrucciones
#define NUMLINES_L1_I 16
#define WORDSPERLINE_L1_I 4
#define ASSOCIATIVITY_L1_I 1// 1 = directa, 2 = 2 v�as
#define USEFIFO_L1_I false

// Par�metros de configuraci�n de la cach� L1 de datos
#define WORDSPERLINE_L1_D 2
#define NUMLINES_L1_D 16
#define ASSOCIATIVITY_L1_D 1
#define MAX_SIZE_QUEUE 8

// Par�metros de configuraci�n de la cach� L2
#define NUMLINES_L2 16
#define WORDSPERLINE_L2 4
#define ASSOCIATIVITY_L2 1
#define USEFIFO_L2 true
#define LATENCY_CYCLES_L2 2
#define LATENCY_CYCLES_MEM 3

#endif