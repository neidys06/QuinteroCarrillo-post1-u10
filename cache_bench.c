#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define REPEAT 100

// Medir tiempo de acceso secuencial a array de N bytes
double bench_seq(size_t n_bytes) {
    volatile char *arr = (volatile char *)malloc(n_bytes);
    if (!arr) return -1.0;
    
    memset((void*)arr, 1, n_bytes);
    
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    
    for (int r = 0; r < REPEAT; r++) {
        for (size_t i = 0; i < n_bytes; i++) {
            (void)arr[i]; // Acceso de lectura forzado
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &t1);
    free((void*)arr);
    
    double ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
    return ns / (REPEAT * (double)n_bytes);
}

// Medir tiempo de acceso aleatorio usando índices pre-mezclados (Fisher-Yates)
double bench_rand(size_t n_bytes) {
    size_t n = n_bytes / sizeof(int);
    if (n == 0) n = 1; // Prevenir división por cero en tamaños ultra pequeños
    
    int *arr = (int*)malloc(n * sizeof(int));
    size_t *idx = (size_t *)malloc(n * sizeof(size_t));
    if (!arr || !idx) {
        if (arr) free(arr);
        if (idx) free(idx);
        return -1.0;
    }
    
    for (size_t i = 0; i < n; i++) {
        arr[i] = 1;
        idx[i] = i;
    }
    
    // Mezcla Fisher-Yates
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        size_t tmp = idx[i];
        idx[i] = idx[j];
        idx[j] = tmp;
    }
    
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    
    volatile long sum = 0;
    for (int r = 0; r < REPEAT; r++) {
        for (size_t i = 0; i < n; i++) {
            sum += arr[idx[i]];
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &t1);
    free(arr);
    free(idx);
    
    double ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
    return ns / (REPEAT * (double)n); // ns por elemento accedido
}

int main(void) {
    // Tamaños especificados en la guía de laboratorio
    size_t sizes[] = {
        4 * 1024,          // 4 KB   (L1 Region)
        8 * 1024,          // 8 KB
        16 * 1024,         // 16 KB
        32 * 1024,         // 32 KB
        64 * 1024,         // 64 KB  (L2 Region)
        128 * 1024,        // 128 KB
        256 * 1024,        // 256 KB
        512 * 1024,        // 512 KB
        1024 * 1024,       // 1 MB   (L3 Region)
        4 * 1024 * 1024,   // 4 MB
        16 * 1024 * 1024,  // 16 MB  (RAM Region)
        64 * 1024 * 1024   // 64 MB
    };
    
    int n = sizeof(sizes) / sizeof(sizes[0]);
    srand(time(NULL)); // Inicializar semilla aleatoria
    
    printf("=====================================================\n");
    printf("%-15s %-20s %-20s\n", "Array Size", "Sequential (ns/B)", "Random (ns/element)");
    printf("=====================================================\n");
    
    for (int i = 0; i < n; i++) {
        double lat_seq = bench_seq(sizes[i]);
        double lat_rand = bench_rand(sizes[i]);
        
        if (sizes[i] < 1024 * 1024) {
            printf("%-15zu KB %-20.3f %-20.3f\n", sizes[i] / 1024, lat_seq, lat_rand);
        } else {
            printf("%-15zu MB %-20.3f %-20.3f\n", sizes[i] / (1024 * 1024), lat_seq, lat_rand);
        }
    }
    printf("=====================================================\n");
    
    return 0;
}