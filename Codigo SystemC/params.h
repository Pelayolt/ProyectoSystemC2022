#ifndef PARAMS_H
#define PARAMS_H

// Parámetros de configuración de la caché L1 de instrucciones
constexpr auto ASSOCIATIVITY_L1_I = 1;		// 1 = directa, 2 = 2 vías, X = X vías
constexpr auto NUMLINES_L1_I = 8;
constexpr auto WORDSPERLINE_L1_I = 16;
constexpr auto USEFIFO_L1_I = true;

// Parámetros de configuración de la caché L1 de datos
constexpr auto ASSOCIATIVITY_L1_D = 1;
constexpr auto NUMLINES_L1_D = 8;
constexpr auto WORDSPERLINE_L1_D = 16;
constexpr auto USEFIFO_L1_D = true;
constexpr auto USEWBACK_L1_D = false;		// false = write-through, true = write-back
constexpr auto MAX_QUEUE_SIZE = 4;			// El tamaño mínimo de la cola debe de ser 2 para poder manejar el pipeline

// Parámetros de configuración de la caché L2
constexpr auto ASSOCIATIVITY_L2 = 1;
constexpr auto NUMLINES_L2 = 64;
constexpr auto WORDSPERLINE_L2 = 32;
constexpr auto USEFIFO_L2 = true;
constexpr auto USEWBACK_L2 = false;			// false = write-through, true = write-back
constexpr auto LATENCY_CYCLES_L2 = 10;
constexpr auto LATENCY_CYCLES_MEM = 100;

#endif