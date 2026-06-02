# Trabajo práctico N°1
## Cuarta Parte: Inicialización de la vector table.

### Objetivo
Dejaremos la estructura del vector de interrupciones preparada preparada para manejar interrupciones y excepciones. Aun no vamos a manejar ninguna interrupción.

### Introducción
> **Concepto importante**:
> En ARM, al ocurrir una excepción, la CPU normalmente suspende la ejecución de la secuencia de instrucciones en curso, ya que salta por cada tipo de interrupción a una dirección de memoria pre establecida. El conjunto de estas direcciones se llaman vector de Interrupciones ya que están equiespaciadas, en este caso a 4 bytes de distancia. 

El vector de interrupciones de un procesadore ARM corresponde al siguiente esquema de direcciones:

0x00  Reset
0x04  Undefined
0x08  SVC
0x0C  Prefetch Abort
0x10  Data Abort
0x14  Reserved
0x18  IRQ
0x1C  FIQ

### :hammer_and_wrench: Construcción de la vector Table
En el archivo ```start.S``` incluimos antes del código una seccción nueva, ```.vectors```
```armasm
/*----------------------------------*/
/* Vector table                     */
/*----------------------------------*/
.section .vectors, "ax"

_vectors:
    b _start              /* Reset */
    b .                   /* Undefined */
    b .                   /* SVC */
    b .                   /* Prefetch abort */
    b .                   /* Data abort */
    b .                   /* Reserved */
    b .                   /* IRQ */
    b .                   /* FIQ */
```
Creamos una sección específica para contener los vectores de interrupción.

> :bulb: **El Linker script no se modifica**
> Ya tenemos la línea
>```ld
>       KEEP(*(.vectors))
>```
### Ejecución
Se construye y ejecuta como los experimentos anteriores. La unica diferencia es la presencia del vetor de interrupciones.

>:mag: **Observaciones**:
> Solo hay un salto real en el vector Reset, a la etiqueta ```_start```. El resto de las entradas de la vector table implementan un loop infinito, ya que el caracter ```.``` es elcontador de direcciones del ensamblador, de modo que significa "ésta dirección".

Ejecutar en una consola en el directorio del proyecto el siguiente comando:
```bash
readelf -S kernel.elf
```
Observar las secciones definidas en el código. 
1. ¿Están todas?
2. ¿Que explicación hay para los faltantes?
Aqui llega la hora de elaborar nuestro diagnóstico. Espero respuestas...

> **Conclusiones** 
> La CPU no “busca handlers”, sino que tiene una tabla de direcciones fijas. Cuando ocurre una excepción, fuerza el ```PC``` a uno de esos vectores.
