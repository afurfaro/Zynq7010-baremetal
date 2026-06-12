# Trabajo práctico N°3
## Primera Parte: Interrupciones de Hardware. IRQ. Manejo del Timer default del Cortex A9

### Objetivo
Generar una IRQ periódica usando el Private Timer del Cortex-A9. Para ello vamos a:
Paso 1: Programar el GIC:
* habilitar Distributor
* habilitar CPU Interface
* habilitar IRQ 29 (Private Timer)

Paso 2: Programar el Private Timer:
* Load Register
* Auto Reload
* IRQ Enable
* Timer Enable

Paso 3: Modificar irq_handler
Reemplazar en **`start.S`** el handler de **`IRQ`** por el siguiente código
```armasm
irq_handler:
    /* acknowledge a la fuente de interrupción */

    /* desachar interrupción */

    subs pc, lr, #4

```
La última instrucción resuelve en forma simultánea las siguientes operaciones:
* `pc = lr - 4`
* Copia **`SPSR_irq → CPSR`**
* Vuelve al modo anterior (el del programa interrumpido). En nuestro caso será el modo **`SYS`** ya que el sistema luego del boot quedó en este modo.

### Manejo del Hardware 
Esto es lo novedoso en este punto de nuestro proyecto. Comenzamos a trabajar con el sistema de Entrada/Salida.
Iniciamos con dos dispositivos: el Private Timer del Cortex A9, al que configuraremos con el objetivo que genere una interrupción **`IRQ`** en forma periódica, y el _Generic Interrupt Controller_ de ARM, de ahora en mas el **GIC**, que gestionará, la interrupción del Private Timer, y en la medida en que vayamos incorporando nuevos dispositivos de E/S, gestionará además sus interrupciones.

#### ¿Qué es el GIC?

El **GIC** es el Controlador de Interrupciones que ARM diseña para sus procesadores. El rol de un controlador de interrupciones, es centralizar la gestión de todas las interrupciones de un sistema de cómputo. En el Zynq-7000, el **GIC** implementa la especificación **ARM GICv1**. 

Su función es actuar como árbitro entre las fuentes de interrupción de los dispositivos de hardware y la/s CPU del sistema. Sin él, cada periférico necesitaría su propio mecanismo de señalización hacia la/s CPU. Y la/s CPU necesitaría/n una cantidad enorme de entradas de interrupción mas la lógica para gestionarlas. Inviable.

Se trata de un periférico más del sistema de E/S, pero su rol lo convierte en un periférico escencial dentro de cualquier sistema de cómputo. Como cualquier periférico, el **GIC** requiere ser inicializado, antes de ponerse en operación y una vez operativo, permite ser controlado y monitoreado por parte del software de gestión. Para ello tiene una cantidad de registros definidos en la espcificación. Xilinx, fabricante del Zynq-7000 lo mapea a partir de la dirección base `0xF8F00000`.

El **GIC** está organizado en dos bloque funcionales perfectmente diferenciados:
```
Periféricos / Timers / GPIO / ...
         │
         ▼
   ┌─────────────┐
   │ Distributor │   ← Decide qué interrupciones están habilitadas
   │   (GICD)    │     y a qué CPU se dirigen
   └──────┬──────┘
          │
   ┌──────▼──────┐
   │CPU Interface│   ← Cada núcleo tiene la suya propia
   │   (GICC)    │     Negocia prioridad, ACK, EOI
   └─────────────┘
         │
         ▼
      CPU Core
```
El **GIC** como se ve en el gráfico anterior se divide en dos partes. 
##### Distributor — GICD (`0xF8F01000`)
El Distributor es **global**, esto es, está compartido por todos los procesadores del sistema. En el caso del Zync-7000, por ambos cores A9. Es responsable de:
- Habilitar/deshabilitar interrupciones individuales
- Asignar prioridades
- Dirigir cada interrupción a uno o ambos cores (en la jerga del **GIC** denominado target)
- Clasificar el modo de disparo de las entradas de interrupción (nivel / flanco)
##### CPU Interface — GICC (`0xF8F00100`)
La CPU Interface es **privada por core**. Es responsable de:
- Habilitar la recepción de interrupciones en ese core
- Establecer el umbral de prioridad mínima
- Proveer el ID de la interrupción activa (normalmente a requerimiento del handler)
- Recibir el **EOI** (End Of Interrupt) para liberar la línea de interrupción
##### Tipos de interrupción según el GIC

| Tipo | Sigla | Rango de IDs | Descripción |
|------|-------|--------------|-------------|
| Software Generated | SGI | 0–15 | Inter-CPU, generadas por software |
| Private Peripheral | PPI | 16–31 | Privadas de cada core (timers, watchdog) |
| Shared Peripheral | SPI | 32–1019 | Periféricos del SoC (UART, Ethernet, etc.) |

> :bangbang: **El Private Timer del Cortex-A9 usa la interrupción ID 29 (PPI).**

##### Registros del Distributor
El Distributor tiene varios registros memory mapped. Vamos a describir lso que son de uso par anuestra implementación. Siempre está la documentación de ARM para poder indagar acerca del resto si es necesario.

| Offset |  Name  | Type |   Reset  | Description |
|--------|--------|------|----------|-------------|
| 0x000  | ICDDCR |  RW  |0x00000000|Distributor Control Register |
| 0x004	 |ICDICTR	|  RO	 |Impl. def.|	Interrupt Controller Type Register |
|0x100-0x17C |ICDISER |  RW |Impl. def.| Interrupt Set-Enable Registers|
|0x400-0x7F8 | ICDIPR | RW  |0x00000000| Interrupt Priority Registers |
|0x800-0x81C | ICDIPTR| RO  |Impl. def.| Interrupt Processor Targets Registers|

###### Distributor Control Register (ICDDCR)
Permite el envío de interrupciones pendientes a las interfaces de la CPU.
```
[31:1]    -      — Reservados. 
[0]     ENABLE   — 0: El GIC ignora todas las señales de interrupción de los periféricos 
                      y no reenvía las interrupciones pendientes a la/s CPU interface.
                   1: El GIC monitorea las señales de interrupción de los periféricos y  
                      envía las interrupciones pendientes a la/s CPU interface.
```
###### Interrupt Controller Type Register (ICDICTR)
Provee información sobre la configuración del **GIC**. Indica:
* Si el **GIC** implementa las Extensiones de Seguridad
* El número máximo de identificadores de interrupción que admite el **GIC**
* El número de interfaces de CPU implementadas
* Si el **GIC** implementa las Extensiones de Seguridad, el número máximo de interrupciones periféricas compartidas bloqueables (LSPI) implementadas.
```
[31:16]       -       Reservado.
[15:11]       -       Si el GIC no implementa las Extensiones de Seguridad, este campo está reservado.
[15:11]      LSPI     Si el GIC implementa las Extensiones de Seguridad, el valor de este campo es el 
                      número máximo de SPI bloqueables implementadas, de 0 (0b00000) a 31 (0b11111). 
                      Consultar Bloqueo de configuración. Si este campo es 0b00000, el GIC no implementa 
                      el bloqueo de configuración.
[10]     SecurityExtn Indica si el GIC implementa las Extensiones de Seguridad.
                      0: Extensiones de seguridad no implementadas.
                      1: Extensiones de seguridad implementadas.
[9:8]         -       Reservado.
[7:5]     CPUNumber   Indica el número de CPU interface implementadas. El número de CPU interface
                      implementadas es el valor de este campo + 1. 
                      Por ejemplo, si este campo es 0b011, hay cuatro CPU interface.
[4:0]   ITLinesNumber Indica el número máximo de interrupciones que admite el GIC [1]. Si el valor de este
                      campo es N, el número máximo de interrupciones es 32(N+1). El rango de interrupt ID 
                      va de 0 a uno menos que el número de interrupt ID. Por ejemplo: 0b00011 Hasta 128 
                      líneas de interrupción, ID de interrupción del 0 al 127.
                      El número máximo de interrupciones es 1020 (0b11111).
                      Consultar el texto de esta sección para obtener más información.

[1] Independientemente del rango de interrupt ID definidos por este campo, los Interrupt ID del 1020 al 1023 están reservados para fines especiales.
```

###### Interrupt Set-Enable Registers (ICDISERn)
Proporcionan un bit de habilitación para cada interrupción compatible con el **GIC**. Escribir un 1 en un bit de habilitación habilita el envío de la interrupción correspondiente a la/s CPU interface. Leer un bit permite saber si la interrupción está habilitada.
El número de **`ICDISER`** implementados es (**`ICDICTR.ITLinesNumber`** + 1). Se debe consultar el Interrupt Controller Type Register (**`ICDICTR`**). Los **`ICDISER`** implementados se numeran a partir de **`ICDISER0`**.

En una implementación multiprocesador, **`ICDISER0`** se banquea para cada procesador conectado. Este registro almacena los bits de habilitación para las interrupciones 0-31.

```
                          │  Para SPIs y PPIs, cada bit:
                          │  Lectura:
                          │      0 Interrupción correspondiente deshabilitada.
[31:0]  Set-enable bits   │      1 Interrupción correspondiente habilitada.
                          │  Escritura:
                          │      0 No tiene efecto.
                          │      1 Habilita la interrupción correspondinente. La subsiguiente lectura de este bit retorna 
                                   el valor 1.


Para las SGIs el comportamiento de este bit en lecturas y escrituras es implementación dependiente.
```
Para un interrupt ID = N, y si  DIV y MOD representan las operaciones de división entera y módulo se tiene:
* El correspondiente número **M** de **`ICDISER`**, se obtiene mediante **M = N DIV 32**.
* El desplazamiento del **ICDISER** requerido es **(0x100 + (4*M))**.
* El bit de habilitación requerido en este registro es **N MOD 32**.

Al arranque y luego de un reset, un procesador puede usar este registro para determinar qué interrupt ID de periféricos admite el** GIC**.

**_Nota_**: Deshabilitar una interrupción solo desactiva su envío a cualquier CPU interface. No impide que la interrupción cambie de estado, por ejemplo, que pase a estar pendiente o activa y pendiente si ya está activa.

###### Interrupt Priority Registers (ICDIPRn)
Los registros **ICDIPR** proporcionan un campo de prioridad de 8 bits para cada interrupción compatible con el **GIC**. Este campo almacena la prioridad de la interrupción correspondiente.
* Estos registros son accesibles por byte.
* Un campo de registro correspondiente a una interrupción no implementada es RAZ/WI (Read As Zero/Write Ignored).
* Un **GIC** puede implementar menos de 8 bits de prioridad, pero debe implementar al menos los bits [7:4] de cada campo. En cada campo, los bits no implementados son RAZ/WI.
La implementación define si el cambio del valor de un campo de prioridad modifica la prioridad de una interrupción activa. 

Configuraciones

Estos registros están disponibles en todas las configuraciones del **GIC**. Si el **GIC** implementa las Extensiones de Seguridad, estos registros son comunes.

El número de **ICDIPR** implementados es **(8 * (`ICDICTR.ITLinesNumber` + 1))**, (ver **Interrupt Controller Type Register (ICDICTR)**). Los **ICDIPR** implementados se numeran a partir de **ICDIPR0**.

En una implementación multiprocesador, los registros **ICDIPR0** a **ICDIPR7** se banquean por cada procesador conectado.Estos registros contienen los campos de Prioridad para las interrupciones 0-31.

```
[31:24]	Prioridad, 
        byte offset 3	
[23:16]	Prioridad, 
        byte offset 2
                          Cada campo de prioridad mantiene un valor de prioridad de un rango
                          "implementation defined". A menor valor, mayor prioridad para la 
                          correspondente interrupción.
[15:8]  Prioridad, 
        byte offset 1
[7:0]   Prioridad, 
        byte offset 0
```
Para un interrupt ID N, si DIV y MOD representan las operaciones de división entera y módulo se tiene:
* El número M del **ICDIPR** correspondiente, se obtiene mediante **M = N DIV 4**.
* El Offset del **ICDIPR** requerido respecto de la dirección Base del Distributor,  es **(0x400 + (4*M))**.
* El byte offset del campo de Prioridad requerido en este registro es **N MOD 4**, donde:
  - El byte offset 0 corresponde a los bits [7:0] del registro.
  - El byte offset 1 corresponde a los bits [15:8] del registro.
  - El byte offset 2 corresponde a los bits [23:16] del registro.
  - El byte offset 3 corresponde a los bits [31:24] del registro.

###### Interrupt Processor Targets Registers (ICDIPTRn)
Los registros **ICDIPTR** proporcionan un campo de CPU destino de 8 bits para cada interrupción compatible con el **GIC**. Este campo almacena la lista de procesadores a los que se envía la interrupción en caso de activarse.
Para una implementación multiprocesador:
* Estos registros son accesibles por byte.
* Un campo de registro correspondiente a una interrupción no implementada es RAZ/WI (Read As Zero/Write Ignored).
* Los registros **ICDIPTR0** a **ICDIPTR7** son de solo lectura, y cada campo devuelve un valor que corresponde únicamente al procesador que lee el registro.
Cada implementación define qué SPI, si las hay, se configura estáticamente en el hardware. El campo CPU target para dicha SPI es solo lectura y devuelve un valor que indica las CPU target para la interrupción.
En una implementación monoprocesador, todas las interrupciones se dirigen a un único procesador y los **ICDIPTR** son RAZ/WI.
El número de **ICDIPTR** implementados es **(8 * (`ICDICTR.ITLinesNumber` + 1))** (ver **Interrupt Controller Type Register (ICDICTR)**). Los **ICDIPTR** implementados se numeran a partir de **ICDIPTR0**.

En una implementación multiprocesador, los registros **ICDIPTR0** a **ICDIPTR7** se banquean por cada procesador conectado. Estos registros contienen los campos CPU target para las interrupciones 0-31.

```
[31:24]	CPU target, 
        byte offset 3	
[23:16]	CPU target, 
        byte offset 2
                          Los procesadores del sistema se numeran a partir del 0, y cada bit en un campo `CPU target` se refiere al procesador correspondiente (véase la Tablasiguiente). Por ejemplo, un valor de 0x3 significa que la interrupción pendiente se envía a los procesadores 0 y 1.
                          Para ICDIPTR0 a ICDIPTR7, la lectura de cualquier campo CPU target devuelve el número del procesador que realiza la lectura.
[15:8]	CPU target, 
        byte offset 1
[7:0]	  CPU target, 
        byte offset 0
```

|Valor del campo CPU targets	| Interrupt targets |
|-----------------------------|-----------------|
|0bxxxx xxx1 |	CPU interface 0 |
|0bxxxx xx1x |	CPU interface 1 |
|0bxxxx x1xx |	CPU interface 2 |
|0bxxxx 1xxx |	CPU interface 3 |
|0bxxx1 xxxx |	CPU interface 4 |
|0bxx1x xxxx |	CPU interface 5 |
|0bx1xx xxxx |	CPU interface 6 |
|0b1xxx xxxx |	CPU interface 7 |

##### Registros de la CPU Interface
La misma aclaración vale para los registros de la/s CPU Interface:

| Offset |  Name  | Type |   Reset  | Description |
|--------|--------|------|----------|-------------|
| 0x00	| ICCICR	| RW	| 0x00000000	| CPU Interface Control Register |
| 0x04	| ICCPMR	| RW	| 0x00000000	| Interrupt Priority Mask Register |
| 0x0C	| ICCIAR	| RO	| 0x000003FF	| Interrupt Acknowledge Register | 
| 0x10	| ICCEOIR	| WO	|      -	| End of Interrupt Register |

###### CPU Interface Control Register (ICCICR)
Permite la señalización de interrupciones a los procesadores de destino.

```
[31:1]      -     Reserved.
[0]     Enable    Habilitación Global para la señalización de interrupciones por parte de las CPU interface a los  procesadores conectados.
                    0: Desabilita señalización de interrupciones.
                    1: Habilita señalización de interrupciones.

En un GIC que implementa Extensiones de Seguridad, este bit controla solo la señallización de interrupciones Non-seguras.######Interrupt Priority Mask Register (ICCPMR)


```
###### Interrupt Priority Mask Register (ICCPMR)
Proporciona un filtro de prioridad de interrupciones. Solo aquellas interrupciones con una prioridad superior al valor de este registro pueden ser señalizadas al procesador.
**Nota:** Una prioridad alta corresponde a un valor menor en el campo `Prioridad`.

```
[31:8]      -       Reserved.
[7:0]   Priority    Nivel de máscara de prioridad para la CPU interface. Si la prioridad de 
                    una interrupción es superior al valor indicado en este campo, la interfaz 
                    envía la interrupción al procesador.
                    Si el GIC soporta menos de 256 niveles de prioridad, algunos bits son RAZ/WI,
                    según se indica a continuación:
                    Soporta 128 niveles
                          Bit [0] = 0.
                    Soporta 64 niveles
                          Bit [1:0] = 0b00.
                    Soporta 32 niveles
                          Bit [2:0] = 0b000.
                    Soporta 16 niveles
                          Bit [3:0] = 0b0000.
```

###### Interrupt Acknowledge Register (ICCIAR)
El handler de interrupción que ejecuta el procesador lee este registro para obtener el identificador de la interrupción que se ha señalado. Esta lectura actúa además como una confirmación de la interrupción.
```
[31:13]       -       Reserved.
[12:10]     CPUID     Para las SGI en una implementación multiprocesador, este campo identifica 
                      el procesador que solicitó la interrupción. Devuelve el número de CPU interface que realizó la solicitud; por ejemplo, un valor de 3 (0b011) significa que la solicitud fue generada por una escritura en el IDCSFGIR en la interfaz 3 de la CPU.
                      Para todas las demás interrupciones, este campo es RAZ (Read As Zero).
[9:0]      ACKINTID   El Interrupt ID.
```
Una lectura del **`ICCIAR`** devuelve el Interrupt ID de la interrupción pendiente de mayor prioridad para la CPU Interface. Devuelve un Interrupt ID espúreo (1023) si se cumple alguna de las siguientes condiciones:
* La señalización de interrupciones a la interfaz de la CPU está deshabilitada.
* No hay ninguna interrupción pendiente en esta CPU Interface con la prioridad suficiente para que la interfaz la envíe al procesador.

**Nota:**
La siguiente secuencia de eventos es un ejemplo de cuándo el **GIC** devuelve un Interrupt ID igual a 1023, y muestra cómo las lecturas del **`ICCIAR`** pueden ser críticas en cuanto a la temporización:
1. Un periférico activa una interrupción por su terminal configurado como activo por nivel.
2. La interrupción tiene prioridad suficiente y, por lo tanto, el **GIC** la envía al procesador target.
3. El periférico desactiva la interrupción (baja el nivel en el terminal de interrupción del Distributor). Dado que no hay ninguna otra interrupción pendiente con prioridad suficiente, el **GIC** desactiva la solicitud de interrupción al procesador.
4. Antes de que el procesador target haya reconocido la desactivación de la solicitud de interrupción del paso anterior, lee el **`ICCIAR`**. Como no hay ninguna interrupción con prioridad suficiente para enviar al procesador, el **GIC** devuelve el Interrupt ID erróneo de 1023.
Un Interrupt ID no espurio devuelto por una lectura del **`ICCIAR`** se denomina Interrupt ID válido.

Cuando el **GIC** devuelve un Interrupt ID válido a una lectura del **`ICCIAR`**, la lectura se interpreta como una confirmación de dicha interrupción y, como efecto secundario, cambia el estado de la interrupción de pendiente a activa, o a activa y pendiente si el estado pendiente persiste. Normalmente, el estado pendiente de una interrupción persiste solo si la entrada de interrupción es activa por nivel y permanece activa.

Por cada lectura de un interpreta válido del **`ICCIAR`**, la rutina de servicio de interrupción del procesador conectado debe realizar una escritura correspondiente en el **`ICCEOIR`** (véase Registro de Fin de Interrupción (ICCEOIR)).

###### End of Interrupt Register (ICCEOIR)
Un procesador escribe en este registro para informar a la CPU Interface que ha completado su rutina de servicio de interrupción para la interrupción especificada.
```
[31:13]       -       Reserved.
[12:10]     CPUID     En una implementación multiprocesador, al finalizar el procesamiento de un
                      SGI, este campo contiene el valor CPUID del acceso ICCIAR correspondiente.
                      En cualquier otro caso, SBZ.
[9:0]     EOIINTID    El valor ACKINTID del  correspondiente acceso ICCIAR.
```
Escribir en este registro hace que el **GIC** cambie el estado de la interrupción identificada:
* a inactiva, si estaba activa;
* a pendiente, si estaba activa y pendiente.

La rutina de servicio de interrupción del procesador conectado debe escribir en el **`ICCEOIR`** por cada lectura de un Interrupt ID válido del **`ICCIAR`** (ver Registro de Reconocimiento de Interrupción [**`ICCIAR`**]). El valor escrito en el **`ICCEOIR`** debe ser el Interrupt ID leído del **`ICCIAR`**.

**Nota:**
Para garantizar la compatibilidad con posibles extensiones de la especificación de la arquitectura **GIC**, ARM recomienda que el software conserve el valor completo del registro leído del **`ICCIAR`** al reconocer la interrupción y que lo escriba de nuevo en el **`ICCEOIR`** una vez finalizado el procesamiento de la misma.

Si una lectura del **`ICCIAR`** devuelve Interrupt ID espurio, el software no tiene que escribir en el **`ICCEOIR`**. Si el software escribe el Interrupt ID espurio en el **`ICCEOIR`**, el **GIC** ignora dicha escritura.

Para interrupciones anidadas, el orden de finalización de la interrupción debe ser el inverso al orden de confirmación de la interrupción. Es decir, el orden de escritura en el **`ICCEOIR`** debe ser el inverso al orden de lectura del **`ICCIAR`**.

Si los valores escritos en el **`ICCEOIR`** no coinciden con una interrupción activa, el efecto de la escritura es impredecible.


#### El Private Timer del Cortex-A9

Cada core A9 tiene su propio timer privado. Sus registros se definen en el Technical Reference del Cortex-A9. En el Zynq-7000 se los ubica a partir de la Base address: **`0xF8F00600`**

Características principales:
- Clock: operan a la mitad del clock de la CPU (`CPU_3x2x / 2`)
- Cuenta descendente (*count-down*), es decir, se decrementan por cada clock de entrada
- Como consecuencia de la carectarística anterior, genera la interrupción **ID 29** al llegar a **cero**
- Puede operar en modo **one-shot** o **auto-reload**

##### Registros del Private Timer

| Offset | Nombre | Descripción |
|--------|--------|-------------|
| `0x00` | `PTIMER_LOAD` | Valor de recarga. Se copia en el counter al inicio o al llegar a 0 en auto-reload |
| `0x04` | `PTIMER_COUNTER` | Valor actual del contador (se decrementa) |
| `0x08` | `PTIMER_CONTROL` | Control: enable, auto-reload, IRQ enable, prescaler |
| `0x0C` | `PTIMER_ISR` | Interrupt Status: bit 0 = evento pendiente. **Escribir 1 para limpiar** (W1C) |

##### PTIMER_CONTROL 
En el Technical Reference del Cortex A9 se lo denomina **_Private Timer Control Register_**. Su bit layout es el siguiente:
```
[31:16] UNK/SBZP     — UNKnown on reads, Should-Be-Zero-or-Preserved on writes. 
[15:8]  PRESCALER    — Divisor adicional del clock (0 = sin división)
[7:3]   UNK/SBZP     — UNKnown on reads, Should-Be-Zero-or-Preserved on writes.
[2]     IRQ_ENABLE   — 1: genera interrupción al llegar a 0
[1]     AUTO_RELOAD  — 1: recarga automática desde PTIMER_LOAD
[0]     TIMER_ENABLE — 1: activa el contador
```
##### PTIMER_ISR 
En el Technical Reference del Cortex A9 se lo denomina **_Private Timer Interrupt Status Register_**. Su bit layout es el siguiente:
```
[31:1] UNK/SBZP   — UNKnown on reads, Should-Be-Zero-or-Preserved on writes. 
[0]    Event Flag — Es un bit persistente que se activa automáticamente cuando el registro del contador 
                    llega a cero. Si la interrupción del Private Timer está habilitada, la interrupción con 
                    ID 29 toma estado pendiente en el distribuidor de interrupciones después de que 
                    se active el Event Flag. El Event Flag se borra cuando se escribe en 1.
```
##### PTIMER_LOAD
En el Technical Reference del Cortex-A9 se lo denomina **_Private Timer Load Register_**. Contiene el valor copiado al **_Primary Timer Counter Register_** cuando este llega a cero con el modo de recarga automática habilitado. Escribir en este registro significa escribir tambien en **_Primary Timer Counter Register_**.

##### PTIMER_COUNTER
El Tecnical Reference del Cortex A9 lo refiere como **_Primary Timer Counter Register_**, y su principal característica es que es un contador decreciente. Para ello es necesario habilitarlo mediante el bit [0] del **_Private Timer Control Register_**. Si el procesador Cortex-A9 está en estado debug, el contador solo decrementa cuando el procesador vuelve al estado normal.
Cuando el **_Primary Timer Counter Register_** llega a cero y el se habilitó el modo Auto reload, recarga el valor en el **_Private Timer Load Register_** y continúa decrementando desde ese valor. Si el modo Auto reload no está habilitado, al  llegar a cero, se detiene.
Cuando el **_Private Timer Load Register_** llega a cero, si la generación de interrupciones está habilitada en el bit [2] del **_Private Timer Control Register_**, se activa el Event Flag del **_Private Timer Interrupt Status Register_**, y la interrupción con ID 29 toma estado Pendiente en el Distribuidor del **GIC**, .
Escribir en el **_Primary Timer Counter Register_** o en el **_Private Timer Load Register_**, fuerza al primero a decrementar desde el valor recién escrito.

---

#### Mapa de registros resumido

```
0xF8F00000  ┌──────────────────────────┐
            │   CPU Interface (GICC)   │
0xF8F00100  │   ICCICR                 │  Enable CPU interface
0xF8F00104  │   ICCPMR                 │  Priority mask
0xF8F0010C  │   ICCIAR                 │  Interrupt ACK (read)
0xF8F00110  │   ICCEOIR                │  End Of Interrupt (write)
            ├──────────────────────────┤
0xF8F00600  │   Private Timer          │
0xF8F00600  │   PTIMER_LOAD            │  Valor de recarga
0xF8F00604  │   PTIMER_COUNTER         │  Valor actual
0xF8F00608  │   PTIMER_CONTROL         │  Enable / IRQ / prescaler
0xF8F0060C  │   PTIMER_ISR             │  Flag de evento (W1C)
            ├──────────────────────────┤
0xF8F01000  │   Distributor (GICD)     │
0xF8F01000  │   ICDDCR                 │  Enable distributor
0xF8F01100  │   ICDISER0               │  Enable IRQs 0–31
0xF8F01420  │   ICDIPR7                │  Prioridad IRQ 28–31
0xF8F01820  │   ICDIPTR7               │  Target CPU IRQ 28–31
            └──────────────────────────┘
```

> El offset exacto de los registros de prioridad y target para IRQ 29:
> - `GICD_IPRIORITYR`: base `0x400` + byte `29` → `0xF8F0141D`
> - `GICD_ITARGETSR`: base `0x800` + byte `29` → `0xF8F0181D`

---

#### Secuencia de configuración
Para lograr que el Private Timer genere interrupciones de manera periódica, hay que configurar **tres capas** en estricto orden:

```
1. Distributor   →  habilitar y configurar la IRQ ID 29
2. CPU Interface →  habilitar el core para recibir IRQs
3. Private Timer →  configurar período y habilitar la IRQ del timer
```

##### Configurar el Distributor (GICD)

**Registros a configurar:**

| Registro | Dirección | Qué hacer |
|----------|-----------|-----------|
| `ICDDCR` | `0xF8F01000` | Escribir `1` → habilitar el Distributor |
| `ICDISERn` | `0xF8F01100 + (n*4)` | Setear el bit correspondiente a IRQ 29 |
| `ICDIPRn` | `0xF8F01400 + (n*4)` | Asignar prioridad (ej. `0xA0`) al ID 29 |
| `ICDIPTRn` | `0xF8F01800 + (n*4)` | Indicar a qué CPU se dirige (CPU0 = `0x01`) |

> **Cálculo del índice:** Para IRQ ID `N`:
> - Registro n = `N / 32`, bit = `N % 32` → para ICDISER
> - Byte offset = `N` → para ICDIPR e ICDIPTR (1 byte por IRQ)

**IRQ 29 en ICDISER:**
- Registro: `ICDISER0` (`0xF8F01100`)
- Bit a setear: `1 << 29`

##### Configurar la CPU Interface (GICC)

| Registro | Dirección | Qué hacer |
|----------|-----------|-----------|
| `ICCICR` | `0xF8F00100` | Escribir `1` → habilitar la CPU interface |
| `ICCPMR` | `0xF8F00104` | Priority Mask: escribir `0xFF` para permitir todas las prioridades |

> `ICCPMR` actúa como un umbral: solo pasan interrupciones con prioridad **menor** (numéricamente) al valor escrito. Con `0xFF` se habilitan todas.

##### Configurar el Private Timer

```c
#define PTIMER_BASE     0xF8F00600
#define PTIMER_LOAD     (*(volatile uint32_t*)(PTIMER_BASE + 0x00))
#define PTIMER_COUNTER  (*(volatile uint32_t*)(PTIMER_BASE + 0x04))
#define PTIMER_CONTROL  (*(volatile uint32_t*)(PTIMER_BASE + 0x08))
#define PTIMER_ISR      (*(volatile uint32_t*)(PTIMER_BASE + 0x0C))

void private_timer_init(uint32_t load_value) {
    PTIMER_LOAD    = load_value;         // período
    PTIMER_CONTROL = (0x00 << 8)  |     // prescaler = 0
                     (1 << 2)     |     // IRQ enable
                     (1 << 1)     |     // auto-reload
                     (1 << 0);          // timer enable
}
```

##### El handler de interrupción

En el handler es **obligatorio**:
1. Leer `GICC_IAR` (`0xF8F0010C`) para obtener el ID de la interrupción activa (y señalizar al GIC que fue tomada).
2. Ejecutar la lógica de la aplicación.
3. Limpiar el flag del timer: escribir `1` en `PTIMER_ISR`.
4. Escribir el mismo ID en `GICC_EOIR` (`0xF8F00110`) para el End Of Interrupt.

```c
void IRQ_Handler(void) {
    uint32_t irq_id = GICC_IAR;          // ACK → obtiene ID (29)

    if ((irq_id & 0x3FF) == 29) {        // máscara de 10 bits
        // ... lógica de la aplicación ...
        PTIMER_ISR = 1;                  // W1C: limpiar flag del timer
    }

    GICC_EOIR = irq_id;                  // EOI → liberar al GIC
}
```

> ⚠️ Si se omite el EOI, el GIC considera la interrupción como todavía activa y no entregará nuevas interrupciones de igual o menor prioridad. En QEMU esto se manifiesta como una sola interrupción recibida y luego silencio.


---

### :hammer_and_wrench: Implementación

#### :open_file_folder: Estructura del proyecto
Este proyecto requiere organizar los fuentes en diferentes archivos, e introducir diversos headers para cada nuevo hardware que tengamos que incorporar, en donde se definan las macros.
Esto es necesario a fin de elaborar una estructura escalable, ordenada, y fácilmente legible.
Al tener ya multiples archivos fuente, para que el build sea ordenado deberíamos definir la estructura de directorios del proyecto de aqui en mas:
```text
.
├── include/      # Archivos .h
├── src/          # Archivos .c y .S
├── obj/          # Objetos intermedios
├── bin/          # kernel.elf, kernel.bin, kernel.map
├── README.md     # Documentación
├── Makefile      # Modificado para ingresar a cada subdirectorio
└── linker.ld
```

#### Makefile
Modificamos el Makefile sde manera consistente con esta nueva estructura.
El nuevo Makefile tiene las siguientes características:

✔️ Sólo creamos nuevos archivos en los subdirectorios  src/ e include/. 
✔️ obj/ y bin/ se crean automáticamente si no existen (usando mkdir -p).
✔️ Los .o se generan en el subdirectorio obj/, de modo qu eno se mezclan ya con los fuentes y demás archivos.
✔️ kernel.elf, kernel.bin, kernel.map y kernel.lst se generan en el subdirectorio bin/.
✔️ El linker utiliza linker.ld desde el directorio raíz.
✔️ La regla clean elimina únicamente el contenido de los subdirectorios obj/ y bin/, sin tocar ningún archivo de los subdirectorios src/ ni include/.
✔️ Las dependencias se generan automáticamente para recompilar sólo lo necesario cuando cambie un .h.
✔️ Agregar nuevos .c o .S no requiere modificar el Makefile (esta funcionalidad ya estaba, simplemente se mantiene).

#### Archivos fuente
Los archivos **`start.S`** y **`kernel.c`** se mueven al subdirectorio **`src/`**. Se suman en dicho subdirectorio y en **`include/`** en donde ya tenemos **`armv7.inc`**.

```text
include/
    armv7.inc
    gic.h
    timer.h
    kernel.h

src/
    gic.c
    timer.c
    start.S
    kernel.c
```
---

#### Macros
En los sistemas embedded es muy frecuente que los registros de control de los periféricos se accedan **mapeados en memoria ** (*memory mapped I/O*). Desde el punto de vista del procesador, un registro del timer o del controlador de interrupciones no es más que una dirección de memoria que se puede leer o escribir.
Para facilitar la escritura del código definimos en **`kernel.h`** la siguiente macro:
```c
#define REG32(addr) (*(volatile uint32_t *)(addr))
```
Su funcionamiento puede comprenderse analizando la expresión de derecha a izquierda.
En primer lugar, **`(uint32_t *)`** convierte (cast) el valor numérico **`addr`** en un **puntero a un entero no signado de 32 bits**. Es decir, el compilador dejará de interpretar **`addr`** como un simple número y pasa a tratarlo como una dirección de memoria a partir de la cual se encuentra almacenado un dato de 32 bits.

El _keyword_ **`volatile`** antepuesto al cast, indica al compilador que el contenido de esa dirección puede cambiar por causas externas al programa, por ejemplo debido al hardware. Este es el comporatmiento típico de un dispositivo de E/S ya que por lo general pone disponible a la CPU datos que provienen desde el exterior. Esta _keyword_, hace que el compilador no optimice las lecturas ni las escrituras sobre ese registro, accediendo siempre a la dirección física correspondiente.

Finalmente, el operador **`*`**, antepuesto a la expresión `(volatile uint32_t *)(addr)` desreferencia al puntero, es decir, permite acceder al contenido de la dirección **`addr`**. Como resultado, la macro definida como **`REG32(addr)`** se comporta exactamente igual que una variable de tipo **`uint32_t`**, y termina proporcionando el contenido de una dirección de memoria, aunque en realidad en esa dirección se encuentra mapeado un registro de hardware.

Por medio de esta Macro genérica pueden definirse para cada dispositivo de hardware una macro para cada registro de modo de tratar a cada registro de hardware como una variable simple. Para el Private Timerdel Cortex-A9 lo hacemos en su archivo cabecera personalizado `timer.h`:

```c
#define PTIMER_BASE            0xF8F00600   /*Dirección Base del Private Timer*/

#define PTIMER_LOAD            REG32(PTIMER_BASE + 0x00) /* Private Timer Load Register */
#define PTIMER_COUNTER         REG32(PTIMER_BASE + 0x04) /* Private Timer Count Register */
#define PTIMER_CONTROL         REG32(PTIMER_BASE + 0x08) /* Private Timer Control Register */
#define PTIMER_ISR             REG32(PTIMER_BASE + 0x0C) /* Private Timer Interrupt Status Register */
```
Luego inicializar en el código los registros es tan simple como se muestra en el archivo `timer.c`.
```c
void timer_init(uint32_t load)
{
    /* Valor inicial del contador */

    PTIMER_LOAD = load;

    /*
     * Control Register
     *
     * bit0 = ENABLE
     * bit1 = AUTO_RELOAD
     * bit2 = IRQ_ENABLE
     * bits[15:8] = PRESCALER
     */

    PTIMER_CONTROL = (1 << 0) |
                     (1 << 1) |
                     (1 << 2);

    /* Limpiar una posible interrupción pendiente */

    PTIMER_ISR = 1;
}
```
puede interpretarse conceptualmente en cuatro pasos:

1. Se calcula la dirección del registro del timer sumando la dirección base del periférico `PTIMER_BASE`, y el desplazamiento (*offset*) correspondiente al registro particular. Por ejemplo para `LOAD` el offset es `0x00`, para `COUNTER` `0x04`, para `CONTROL` `0x08` y para `ISR` `0x0C`.
2. Cada dirección se convierte en un puntero a entero de 32 bits.
3. Cada puntero se desreferencia para acceder al contenido de esa posición de memoria, o sea de cada Registro de Hardware.  
4. Se escribe el cada valor en dichas direcciones, o sea en los registros. Ejemplo `1000000` en el Load Register, ya que ese es el valor del argumento `load` recibido por la función de inciialización invocada desde kernel.c

En otras palabras, esta sentencia equivale a escribir directamente en un registro físico del timer. El hardware detecta esa escritura y actualiza internamente su registro. 

> :bulb: El procesador no está manipulando memoria RAM convencional, sino interactuando directamente con un periférico mediante el mismo mecanismo de acceso a memoria que utiliza para leer o escribir variables comunes y corrientes.

---




### Verificaciones previas.

Verificamos si qemu puede emular correctamente los recursos que planteamos
De acuerdo con el Technical Reference de Zynq-7000, la dirección Base del Mapa de Registros Privados de la CPU  es 0xF8900000. Nuestro interés en particular es a partir de 0xF8F00000, en donde se encuentran los registros de 
* SCU
* GIC CPU Interface
* GIC Distributor
* Private Timer
* Global Timer
Muchos de los cuales son de nuestro interés

Para verificarlo ingresamos en culquiera de los TPs previos, ponemos un breakpoint en la primer instrucción de `kernel.c`, e inspeccionamos las áreas de memoria siguientes:

```gdb 
>>> x/32wx 0xF8F00000

0xf8f00000:     0x00000000      0x00000010      0x00000000      0x00000000
0xf8f00010:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00020:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00030:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00040:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00050:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00060:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00070:     0x00000000      0x00000000      0x00000000      0x00000000

>>> x/32wx 0xF8F00100
0xf8f00100:     0x00000000      0x00000000      0x00000000      0x000003ff
0xf8f00110:     0x00000000      0x000000ff      0x000003ff      0x00000000
0xf8f00120:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00130:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00140:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00150:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00160:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00170:     0x00000000      0x00000000      0x00000000      0x00000000

>>> x/32wx 0xF8F01000

0xf8f01000:     0x00000000      0x00000002      0x0000043b      0x00000000
0xf8f01010:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f01020:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f01030:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f01040:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f01050:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f01060:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f01070:     0x00000000      0x00000000      0x00000000      0x00000000

>>> x/32wx 0xF8F00600

0xf8f00600:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00610:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00620:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00630:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00640:     Cannot access memory at address 0xf8f00640
```
Con estas salidas confirmamos que QEMU está modelando correctamente el bloque MPCore. Vaos por cada parte del MPCore de interés

#### 1. SCU

En:
```gdb
x/32wx 0xF8F00000
```
aparece:
```text
0xf8f00004: 0x00000010
```
Ese valor es consistente con el registro de configuración del SCU. Por lo tanto se confirma que emula correctamente  la Snoop Control Unit:
```text
SCU = 0xF8F00000
```
---

#### 2. GIC CPU Interface

En la dirección:
```gdb 
x/32wx 0xF8F00100
```

Podemos ver que **`0xf8f0010c: 0x000003ff`**. Este valor coincide perfectamente con el registro **`GICC_IAR`** cuando no hay ninguna IRQ pendiente.
El valor leído corresponde a **`Interrupt ID = 1023`** que significa **`Spurious Interrupt`**.
Así que hemos confirmado **`GIC CPU Interface Base Address= 0xF8F00100`**.

> Cuando no existe ninguna interrupción pendiente, el GIC devuelve el ID 1023 (spurious interrupt).


---
#### 3. GIC Distributor

El comando:
```gdb
x/32wx 0xF8F01000
```
muestra **`0xf8f01004: 0x00000002`**. Este es el valor típico del **`ICTR`** (Typer Register), que indica la cantidad de líneas de interrupción implementadas Es en realidad 32*(N+1), en este caso 32*3 = 96 lineas de interrupción.
Por lo tanto confirmamos que soporta **`GIC Distributor = 0xF8F01000`**

---

#### 4. Private Timer
La salida es coherente con lo que esperaba de QEMU: **`0xF8F00600 ... 0xF8F0063C`** es una región válida y actualmente todos los registros están en cero. Eso es razonable porque todavía no configuramos ningún timer.
Y en particular el mensaje: **`0xf8f00640:     Cannot access memory at address `**, es consistente con que el bloque del Private Timer ocupa exactamente 0x40 bytes.
Por lo tanto podemos dar por confirmado: **`Private Timer Base Address = 0xF8F00600`**

---
#### Estado actual

Los cuatro bloques han sud verificados experiementalmente sobre QEMU:

| Bloque            | Base       |
| ----------------- | ---------- |
| SCU               | 0xF8F00000 |
| GIC CPU Interface | 0xF8F00100 |
| Private Timer     | 0xF8F00600 |
| GIC Distributor   | 0xF8F01000 |

Y es consistente con lo que establecen los Technical Reference de Zync 7000 y de Cortex A9.

---
#### Verificaciones adicionales
Otra comprobación es verificar el ID del Timer, la cual podemos hacer mediante el siguiente comando de **`GDB`**:

```gdb
>>> x/16wx 0xF8F00600

0xf8f00600:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00610:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00620:     0x00000000      0x00000000      0x00000000      0x00000000
0xf8f00630:     0x00000000      0x00000000      0x00000000      0x00000000
>>> 
>>> monitor info qtree

bus: main-system-bus
  type System
  dev: xlnx.ps7-dev-cfg, id ""
    gpio-out "sysbus-irq" 1
    mmio 00000000f8007000/0000000000000100
  dev: pl330, id ""
    gpio-in "" 32
    gpio-out "sysbus-irq" 17
    num_chnls = 8 (0x8)
    num_periph_req = 4 (0x4)
    num_events = 16 (0x10)
    mgr_ns_at_rst = 0 (0x0)
    i-cache_len = 4 (0x4)
    num_i-cache_lines = 8 (0x8)
    boot_addr = 0 (0x0)
    INS = 0 (0x0)
    PNS = 0 (0x0)
    data_width = 64 (0x40)
    wr_cap = 8 (0x8)
    wr_q_dep = 16 (0x10)
    rd_cap = 8 (0x8)
    rd_q_dep = 16 (0x10)
    data_buffer_dep = 256 (0x100)
    mmio 00000000f8003000/0000000000001000
  dev: xlnx-zynq-xadc, id ""
    gpio-out "sysbus-irq" 1
    mmio 00000000f8007100/0000000000000020
  dev: generic-sdhci, id ""
    gpio-out "sysbus-irq" 1
    endianness = 2 (0x2)
    sd-spec-version = 2 (0x2)
    uhs = 0 (0x0)
    vendor = 0 (0x0)
    capareg = 1777074304 (0x69ec0080)
    maxcurr = 0 (0x0)
    pending-insert-quirk = false
    mmio 00000000e0101000/0000000000000100
    bus: sd-bus
      type sdhci-bus
      dev: sd-card, id ""
        spec_version = 2 (0x2)
        drive = ""
  dev: generic-sdhci, id ""
    gpio-out "sysbus-irq" 1
    endianness = 2 (0x2)
    sd-spec-version = 2 (0x2)
    uhs = 0 (0x0)
    vendor = 0 (0x0)
    capareg = 1777074304 (0x69ec0080)
    maxcurr = 0 (0x0)
    pending-insert-quirk = false
    mmio 00000000e0100000/0000000000000100
    bus: sd-bus
      type sdhci-bus
      dev: sd-card, id ""
        spec_version = 2 (0x2)
        drive = ""
  dev: cadence_gem, id ""
    gpio-out "sysbus-irq" 1
    mac = "52:54:00:12:34:57"
    netdev = ""
    revision = 131352 (0x20118)
    phy-addr = 7 (0x7)
    num-priority-queues = 1 (0x1)
    num-type1-screeners = 4 (0x4)
    num-type2-screeners = 4 (0x4)
    jumbo-max-len = 10240 (0x2800)
    mmio 00000000e000c000/0000000000000800
  dev: cadence_gem, id ""
    gpio-out "sysbus-irq" 1
    mac = "52:54:00:12:34:56"
    netdev = "hub0port0"
    revision = 131352 (0x20118)
    phy-addr = 7 (0x7)
    num-priority-queues = 1 (0x1)
    num-type1-screeners = 4 (0x4)
    num-type2-screeners = 4 (0x4)
    jumbo-max-len = 10240 (0x2800)
    mmio 00000000e000b000/0000000000000800
  dev: cadence_ttc, id ""
    gpio-out "sysbus-irq" 3
    mmio 00000000f8002000/0000000000001000
  dev: cadence_ttc, id ""
    gpio-out "sysbus-irq" 3
    mmio 00000000f8001000/0000000000001000
  dev: cadence_uart, id ""
    gpio-out "sysbus-irq" 1
    clock-in "refclk" freq_hz=13.8 MHz
    chardev = ""
    mmio 00000000e0001000/0000000000001000
  dev: cadence_uart, id ""
    gpio-out "sysbus-irq" 1
    clock-in "refclk" freq_hz=13.8 MHz
    chardev = "serial0"
    mmio 00000000e0000000/0000000000001000
  dev: usb-chipidea, id ""
    gpio-out "sysbus-irq" 1
    maxframes = 128 (0x80)
    companion-enable = false
    mmio 00000000e0003000/0000000000001000
    bus: usb-bus.1
      type usb-bus
  dev: usb-chipidea, id ""
    gpio-out "sysbus-irq" 1
    maxframes = 128 (0x80)
    companion-enable = false
    mmio 00000000e0002000/0000000000001000
    bus: usb-bus.0
      type usb-bus
  dev: xlnx.ps7-qspi, id ""
    gpio-out "sysbus-irq" 5
    num-busses = 2 (0x2)
    num-ss-bits = 2 (0x2)
    num-txrx-bytes = 4 (0x4)
    mmio 00000000e000d000/0000000000000100
    mmio 00000000fc000000/0000000002000000
    bus: spi1
      type SSI
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 1 (0x1)
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 0 (0x0)
    bus: spi0
      type SSI
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 1 (0x1)
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 0 (0x0)
  dev: xlnx.ps7-spi, id ""
    gpio-out "sysbus-irq" 5
    num-busses = 1 (0x1)
    num-ss-bits = 4 (0x4)
    num-txrx-bytes = 1 (0x1)
    mmio 00000000e0007000/0000000000000100
    bus: spi0
      type SSI
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 3 (0x3)
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 2 (0x2)
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 1 (0x1)
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 0 (0x0)
  dev: xlnx.ps7-spi, id ""
    gpio-out "sysbus-irq" 5
    num-busses = 1 (0x1)
    num-ss-bits = 4 (0x4)
    num-txrx-bytes = 1 (0x1)
    mmio 00000000e0006000/0000000000000100
    bus: spi0
      type SSI
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 3 (0x3)
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 2 (0x2)
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 1 (0x1)
      dev: n25q128, id ""
        gpio-in "WP#" 1
        gpio-in "ssi-gpio-cs" 1
        write-enable = false
        nonvolatile-cfg = 36863 (0x8fff)
        spansion-cr1nv = 0 (0x0)
        spansion-cr2nv = 8 (0x8)
        spansion-cr3nv = 2 (0x2)
        spansion-cr4nv = 16 (0x10)
        drive = ""
        cs = 0 (0x0)
  dev: arm_mptimer, id ""                  @<=========
    gpio-out "sysbus-irq" 1
    num-cpu = 1 (0x1)
    mmio ffffffffffffffff/0000000000000020
    mmio ffffffffffffffff/0000000000000020
  dev: arm_mptimer, id ""                  @<=========
    gpio-out "sysbus-irq" 1
    num-cpu = 1 (0x1)
    mmio ffffffffffffffff/0000000000000020
    mmio ffffffffffffffff/0000000000000020
  dev: arm.cortex-a9-global-timer, id ""   @<=========
    gpio-out "sysbus-irq" 1
    num-cpu = 1 (0x1)
    mmio ffffffffffffffff/0000000000000020
    mmio ffffffffffffffff/0000000000000020
  dev: arm_gic, id ""                      @<=========
    gpio-in "" 96
    num-cpu = 1 (0x1)
    num-irq = 96 (0x60)
    revision = 1 (0x1)
    has-security-extensions = false
    has-virtualization-extensions = false
    num-priority-bits = 5 (0x5)
    mmio ffffffffffffffff/0000000000001000
    mmio ffffffffffffffff/0000000000000100
    mmio ffffffffffffffff/0000000000000100
  dev: a9-scu, id ""
    num-cpu = 1 (0x1)
    mmio ffffffffffffffff/0000000000000100
  dev: a9mpcore_priv, id ""                 @<--------
    gpio-in "" 64
    gpio-out "sysbus-irq" 4
    num-cpu = 1 (0x1)
    num-irq = 96 (0x60)
    mmio 00000000f8f00000/0000000000002000  @<--------
  dev: xilinx-zynq_slcr, id ""
    clock-out "uart1_ref_clk" freq_hz=13.8 MHz
    clock-out "uart0_ref_clk" freq_hz=13.8 MHz
    clock-in "ps_clk" freq_hz=33.3 MHz
    mmio 00000000f8000000/0000000000001000
  dev: cfi.pflash02, id ""
    drive = ""
    num-blocks = 512 (0x200)
    sector-length = 131072 (0x20000)
    num-blocks0 = 512 (0x200)
    sector-length0 = 131072 (0x20000)
    num-blocks1 = 0 (0x0)
    sector-length1 = 0 (0x0)
    num-blocks2 = 0 (0x0)
    sector-length2 = 0 (0x0)
    num-blocks3 = 0 (0x0)
    sector-length3 = 0 (0x0)
    width = 1 (0x1)
    mappings = 1 (0x1)
    big-endian = 0 (0x0)
    id0 = 102 (0x66)
    id1 = 34 (0x22)
    id2 = 0 (0x0)
    id3 = 0 (0x0)
    unlock-addr0 = 1365 (0x555)
    unlock-addr1 = 682 (0x2aa)
    name = "zynq.pflash"
    mmio 00000000e2000000/0000000004000000


>>> monitor info irq

>>> 
```
En info qtree aparece:
```
dev: `9mpcore_priv                         @<---------
    ...
    ...
    mmio 00000000f8f00000/0000000000002000 @<---------
```
y dentro de ese bloque aparecen:
```
dev: arm_gic                      @<=========
dev: arm_mptimer                  @<=========
dev: arm.cortex-a9-global-timer   @<=========
```
Eso significa que QEMU está implementando correctamente el bloque privado del Cortex-A9 en el espacio de direccionamiento **`0xF8F00000 - 0xF8F01FFF`**, que coincide con la docuentación de Zynq 7000.

---
#### Conclusión
Ya tenemos suficiente seguridad para abordar el TP3.1 con la seguridad que el hardware está correctamente emulado en **`qemu`**.

