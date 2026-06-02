# QuinteroCarrillo-Post1-U10
# Laboratorio:**Unidad:** 10 – Memorias del Computador - Arquitectura de Computadores
**Post-Contenido 1 · Ingeniería de Sistemas · UFPS · 2026**

---

## Objetivo

Implementar y ejecutar un benchmark en C que mide la latencia de acceso a memoria para arrays de distintos tamaños, observando empíricamente el efecto de la jerarquía de caché (L1, L2, L3 y RAM) sobre el rendimiento de los programas. Los resultados se interpretan en función de los conceptos de localidad espacial, líneas de caché y *cache miss*.

---

## Entorno de Compilación

| Requisito | Detalle |
|---|---|
| Sistema operativo | Ubuntu 24.04 (WSL2 sobre Windows) |
| Compilador | GCC 13.3.0 (`gcc -O0`) |
| Herramientas | `/sys/devices/system/cpu/cpu0/cache/`, `clock_gettime` |
| Directorio de trabajo | `~/u10post1/` |

---

## Estructura del Repositorio

```
QuinteroCarrillo-post1-u10/
├── cache_bench.c        # Benchmark de latencia secuencial y aleatoria
├── captures/
│   ├── checkpoint1.png  # Tamaños de caché detectados
│   ├── checkpoint2.png  # Tabla de resultados secuencial
│   └── checkpoint3.png  # Comparativa secuencial vs aleatorio
└── README.md
```

---

## Pasos de Ejecución

### 1. Clonar el repositorio y preparar el entorno

```bash
git clone https://github.com/neidys06/QuinteroCarrillo-post1-u10.git
cd QuinteroCarrillo-post1-u10
```

### 2. Compilar el benchmark

```bash
gcc -O0 -o cache_bench cache_bench.c
```

La bandera `-O0` desactiva las optimizaciones del compilador para evitar que el compilador elimine los accesos a memoria que parecen "inútiles" — sin esto, el bucle de benchmark podría ser descartado por completo.

### 3. Ejecutar

```bash
./cache_bench
```

---

## Checkpoint 1 — Topología de Caché del Sistema

Se ejecutó el siguiente comando para detectar los niveles de caché del procesador:

```bash
for i in 0 1 2 3; do
  echo -n "Cache level $i: "
  cat /sys/devices/system/cpu/cpu0/cache/index${i}/size 2>/dev/null || echo "N/A"
done
```

**Resultado obtenido:**

| Índice | Tamaño | Nivel |
|---|---|---|
| 0 | 32 KB | L1 (datos) |
| 1 | 32 KB | L1 (instrucciones) |
| 2 | 512 KB | L2 |
| 3 | 4096 KB (4 MB) | L3 |

Estos valores determinan los **puntos de inflexión** esperados en el benchmark: al cruzar 32 KB el acceso deja de ser servido por L1, al cruzar 512 KB deja L2, y al superar 4 MB los datos deben leerse desde RAM.

![Checkpoint 1](captures/checkpoint1.png)

---

## Checkpoint 2 — Benchmark de Acceso Secuencial

El programa mide la latencia media en nanosegundos por byte (`ns/B`) para acceso secuencial sobre arrays de distintos tamaños.

**Resultados obtenidos (compilado con `-O0`, sin optimizaciones):**

| Tamaño del Array | Acceso Secuencial (ns/B) | Región estimada |
|---|---|---|
| 4 KB | 1.027 | L1 |
| 8 KB | 1.019 | L1 |
| 16 KB | 1.019 | L1 |
| 32 KB | 1.027 | L1 (límite) |
| 64 KB | 1.032 | L2 ↑ |
| 128 KB | 1.036 | L2 |
| 256 KB | 1.030 | L2 |
| 512 KB | 1.035 | L2 (límite) |
| 1 MB | 1.032 | L3 ↑ |
| 4 MB | 1.204 | L3 (límite) |
| 16 MB | 1.065 | RAM ↑ |
| 64 MB | 1.247 | RAM |

**Análisis de los saltos de latencia:**

- **4 KB → 64 KB (transición L1 → L2):** La latencia se mantiene estable (~1.019–1.027 ns/B) mientras el array cabe en L1 (32 KB). Al superar ese tamaño, el procesador empieza a buscar datos en L2, lo que se refleja en una leve elevación. El acceso secuencial se beneficia enormemente de la *prefetching* del hardware, que carga líneas de caché anticipadamente.

- **512 KB → 1 MB (transición L2 → L3):** Al superar L2 (512 KB), los accesos comienzan a resolverse desde L3. La latencia no sube de forma dramática en el acceso secuencial gracias al prefetcher, que predice el patrón lineal.

- **4 MB → 16 MB (transición L3 → RAM):** Al superar los 4 MB de L3 se observa el salto más visible: la latencia sube de 1.032 a 1.204–1.247 ns/B, evidenciando que los accesos ya no se resuelven en ningún nivel de caché y se generan *cache misses* que alcanzan la RAM principal.

> En el acceso secuencial los saltos son relativamente suaves porque el hardware realiza *prefetching*: detecta el patrón lineal y carga los datos con anticipación, amortiguando parcialmente el costo del *miss*.

![Checkpoint 2](captures/checkpoint2.png)

---

## Checkpoint 3 — Acceso Aleatorio vs Secuencial

Se añadió la función `bench_rand()` que utiliza una mezcla Fisher-Yates para generar un orden de acceso completamente aleatorio, eliminando la localidad espacial.

**Resultados comparativos:**

| Tamaño | Secuencial (ns/B) | Aleatorio (ns/elemento) | Diferencia |
|---|---|---|---|
| 4 KB | 1.027 | 1.667 | +62% |
| 8 KB | 1.019 | 1.430 | +40% |
| 16 KB | 1.019 | 1.411 | +38% |
| 32 KB | 1.027 | 1.426 | +39% |
| 64 KB | 1.032 | 1.456 | +41% |
| 128 KB | 1.036 | 1.473 | +42% |
| 256 KB | 1.030 | 1.957 | +90% |
| 512 KB | 1.035 | 1.258 | +22% |
| 1 MB | 1.032 | 2.258 | +119% |
| 4 MB | 1.204 | 8.865 | +636% |
| 16 MB | 1.065 | 14.640 | +1274% |
| 64 MB | 1.247 | 15.586 | +1150% |

**Análisis por tipo de salto:**

**Saltos atribuidos a *cache miss*:**  
A partir de 1 MB la latencia del acceso aleatorio se dispara de 2.258 a 8.865 ns/elemento (al pasar de L3 a RAM). Esto se debe a que cada acceso aleatorio genera un *cache miss* completo: el procesador no puede predecir el siguiente índice, por lo que la línea de caché requerida nunca está precargada. Cada miss implica ir a buscar 64 bytes desde RAM por un único elemento útil.

**Saltos atribuidos a *TLB miss*:**  
El salto más abrupto ocurre al superar ~1–2 MB, que coincide con el rango donde el TLB (*Translation Lookaside Buffer*) deja de poder contener todas las páginas virtuales del array. A partir de ese punto, cada acceso aleatorio puede requerir una traducción de dirección virtual → física que no está en el TLB, añadiendo latencia de *page walk* sobre el costo del *cache miss*. Esto explica el salto brutal de 2.258 ns (1 MB) a 8.865 ns (4 MB) y hasta 15.586 ns (64 MB).

**Conclusión:** El acceso secuencial es hasta **12× más rápido** que el aleatorio para arrays grandes porque:
1. El *prefetcher* del hardware anticipa los accesos lineales.
2. La localidad espacial garantiza que toda una línea de caché (64 B) sea útil en cada carga.
3. El TLB resuelve eficientemente las traducciones de páginas contiguas.

El acceso aleatorio destruye ambas ventajas, generando *cache misses* y *TLB misses* casi en cada acceso cuando el array supera el tamaño del caché.

![Checkpoint 3](captures/checkpoint3.png)

---

## Diagrama de la Jerarquía de Memoria

```
Velocidad   Tamaño
  ↑             ↓
┌─────────────────────────┐
│  Registros  (~0.3 ns)   │  < 1 KB
├─────────────────────────┤
│  L1 Caché   (~1.0 ns)   │  32 KB   ← arrays ≤ 32 KB
├─────────────────────────┤
│  L2 Caché   (~4 ns)     │  512 KB  ← arrays 32–512 KB
├─────────────────────────┤
│  L3 Caché   (~15 ns)    │  4 MB    ← arrays 512 KB–4 MB
├─────────────────────────┤
│  RAM DRAM   (~60 ns)    │  GBs     ← arrays > 4 MB
└─────────────────────────┘
  ↓             ↑
Velocidad   Tamaño
```
**Conclusiones**

La jerarquía de caché tiene un impacto medible y real sobre el rendimiento. Los resultados confirman empíricamente lo que la teoría predice: los accesos a datos que caben en L1 (~1.019 ns/B) son significativamente más rápidos que los que deben ir a RAM (~1.247 ns/B en secuencial y hasta 15.586 ns/elemento en aleatorio). La diferencia no es teórica — se observó directamente en la ejecución del programa.
La localidad espacial es el factor más determinante para el rendimiento de acceso a memoria. El acceso secuencial aprovecha que el hardware carga líneas de caché completas (64 bytes) anticipándose al siguiente acceso (prefetching). Esto amortigua el costo de los cache misses y mantiene la latencia estable incluso para arrays que superan L1 y L2. Diseñar estructuras de datos y algoritmos que recorran memoria de forma lineal es una de las optimizaciones más efectivas disponibles.
El acceso aleatorio es devastador para el rendimiento cuando el array supera el tamaño del caché. Con arrays de 16–64 MB, el acceso aleatorio es hasta 12× más lento que el secuencial. Cada acceso genera un cache miss completo porque el prefetcher no puede predecir el siguiente índice aleatorio, y a partir de ~1–2 MB se suman los TLB misses, que añaden el costo de la traducción de direcciones virtuales a físicas en cada acceso.
Los tamaños de caché del procesador son umbrales de diseño, no solo datos de especificación. Conocer que L1=32 KB, L2=512 KB y L3=4 MB del procesador utilizado permite predecir dónde aparecerán los puntos de inflexión en el rendimiento de cualquier programa que trabaje sobre grandes volúmenes de datos. Este conocimiento es esencial para optimizar algoritmos de procesamiento de imágenes, matrices, bases de datos en memoria o cualquier aplicación de alto rendimiento.
Compilar con -O0 fue indispensable para la validez del experimento. Las optimizaciones del compilador pueden eliminar bucles que parecen no producir efectos visibles, invalidando completamente el benchmark. El uso de volatile complementa esta protección al nivel del lenguaje C.
