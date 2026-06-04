# Trabajo práctico N°3
## Primera Parte: Interrupciones de Hardware. IRQ. Manejo del Timer default del Cortex A9

### Objetivo
Configurar el private timer del Cortex A9 y el GIC para obtener la generación de una interrupción periódica de hardware. Verificar que:
* La CPU abandona el modo normal de ejecución.
* E*ntra en modo IRQ.
* El hardware guarda automáticamente:
    * **`CPSR → SPSR_irq`**
    * Dirección de retorno **`→ LR_irq`**
* Que se ejecuta irq_handler.
* Que se retorna correctamente al programa interrumpido.

### Introducción.
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
``` 
x/32wx 0xF8F01000
```
muestra **`0xf8f01004: 0x00000002`**. Este es el valor típico del **`ICTR`** (Typer Register), que indica la cantidad de líneas de interrupción implementadas Es en realidad 32*(N+1), en este caso 32*3 = 96 lineas de interrupción.
Por lo tanto confirmamos que soporta **`GIC Distributor = 0xF8F01000`**
---

#### 4. Private Timer
La salida es coherente con lo que esperaba de QEMU: **`0xF8F00600 ... 0xF8F0063C`** es una región válida y actualmente todos los registros están en cero. Eso es razonable porque todavía no configuramos ningún timer.
Y en particular el mensaje: **`0xf8f00640:     Cannot access memory at address `**, es consistente con que el bloque del Private Timer ocupa exactamente 0x40 bytes.
Por lo tanto podemos dar por confirmado: **`Private Timer Base Address = 0xF8F00600`**


#### Estado actual

Los cuatro bloques han sud verificados experiementalmente sobre QEMU:

| Bloque            | Base       |
| ----------------- | ---------- |
| SCU               | 0xF8F00000 |
| GIC CPU Interface | 0xF8F00100 |
| Private Timer     | 0xF8F00600 |
| GIC Distributor   | 0xF8F01000 |

Y es consistente con lo que establecen los Technical Reference de Zync 7000 y de Cortex A9.

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

### Conclusión
Ya tenemos evidencia suficiente para arrancar TP3.1 en forma segura.



---

<!-- ## Lo interesante

Ya podemos ir preparando el TP3.1.

De hecho ya tenemos prácticamente el mapa:

```text
0xF8F00000  SCU

0xF8F00100  GIC CPU Interface

0xF8F00600  Private Timer

0xF8F01000  GIC Distributor
```

---

## Siguiente experimento que haría

Sin tocar todavía el timer.

Desde GDB:

```gdb
x/wx 0xF8F0010C
```

Debería devolver:

```text
0x000003ff
```

que es el valor del ICCIAR.

Eso nos sirve para explicar en el README:


Es una observación muy linda para introducir el GIC antes de generar una IRQ real.

---

Ahora me interesa especialmente la salida de:

```gdb
x/32wx 0xF8F00600
```

porque con eso podemos identificar los registros del Private Timer y empezar a escribir la inicialización real del TP3.1. -->