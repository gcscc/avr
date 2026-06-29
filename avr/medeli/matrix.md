| ESP GPIO Pin | Wire color | Function | Header Bass | Header Treble |
| :------------:| :----------:| :--------:| :-----------:| :-------------:|
| 8            | #ff0       | T0       | 2           | 2             |
| 17           | #f00       | T1       | 4           | 4             |
| 15           | #000       | T2       | 6           | 6             |
| 6            | #777       | T3       | 8           | 8             |
|              |            | T4       | 10          | 10            |
|              |            | T5       | 12          | 12            |
|              |            | T6       | 14          | 14            |
|              |            | T7       | 16          | 16            |
|              |            | MK0      | 1           |               |
|              |            | MK1      | 5           |               |
|              |            | MK2      | 9           |               |
|              |            | MK3      | 13          |               |
| 18           | #fa0       | MK4      |             | 1             |
| 7            | #fff       | MK5      |             | 5             |
|              |            | MK6      |             | 9             |
|              |            | MK7      |             | 13            |
|              | #000       | BR0      | 3           |               |
|              | #fff       | BR1      | 7           |               |
|              | #888       | BR2      | 11          |               |
|              | #808       | BR3      | 15          |               |
| 16           | #640       | BR4      |             | 3             |
| 5            | #888       | BR5      |             | 7             |
|              |            | BR6      |             | 11            |
|              | #fa0       | BR7      |             | 15            |
|              | #000       | MK8      |             | 17            |
|              | #880       | MK9      |             | 18            |
|              | #fff       | BR8      |             | 19            |
|              |            | BR9      |             | 20            |

# 74HC166 Actual pins


| IC | Func    | O que é / função                                                                                                                                                             |
| -: | ------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|  1 | SER     | **Serial Input** — entrada serial. Usada para encadear outro registrador ou injetar bits durante deslocamento. Normalmente liga em nível fixo se não estiver usando cascata. |
|  2 | A       | **Entrada paralela A (bit 0)** — um dos sinais que serão capturados no carregamento paralelo.                                                                                |
|  3 | B       | **Entrada paralela B (bit 1)**                                                                                                                                               |
|  4 | C       | **Entrada paralela C (bit 2)**                                                                                                                                               |
|  5 | D       | **Entrada paralela D (bit 3)**                                                                                                                                               |
|  6 | CLK INH | **Clock Inhibit** — trava o clock. Alto = ignora pulsos em CLK. Baixo = permite deslocamento. Geralmente ligado ao GND se não precisar bloquear leitura.                     |
|  7 | CLK     | **Clock** — borda de subida desloca os bits internamente quando em modo shift.                                                                                               |
|  8 | GND     | Terra (0 V).                                                                                                                                                                 |
|  9 | CLR     | **Clear (ativo em nível baixo)** — limpa o registrador de deslocamento (zera saída serial). Em leitura de matriz normalmente fica em nível alto fixo.                        |
| 10 | E       | **Entrada paralela E (bit 4)**                                                                                                                                               |
| 11 | F       | **Entrada paralela F (bit 5)**                                                                                                                                               |
| 12 | G       | **Entrada paralela G (bit 6)**                                                                                                                                               |
| 13 | QH      | **Serial Output** — saída serial dos dados carregados/deslocados. Vai para o ESP ou próximo registrador da cadeia.                                                           |
| 14 | H       | **Entrada paralela H (bit 7)**                                                                                                                                               |
| 15 | SH/LD   | **Shift / Load** — seleciona modo de operação:<br>Baixo → carrega A–H em paralelo<br>Alto → desloca no clock                                                                 |
| 16 | VCC     | Alimentação (3,3 V ou 5 V conforme teu circuito).                                                                                                                            |


# Matrix Interface

## ESP32 ⇄  Shift Registers

| ESP GPIO 	| 74HC595 IO 	| 74HC165 IO 	| 166 | Function 	| Desc                               	| Color 	|
|----------	|------------ |------------	| --- |----------	|------------------------------------	|-------	|
| 3V3      	| 10         	|            	|     | MR       	|                                    	| #f00  	|
| 3V3      	| 16         	| 16         	| 16  | VCC      	|                                    	| #f00  	|
| 3V3      	|            	|            	|     |          	|                                    	| #f00  	|
| 3V3      	|            	|            	| 9   | CLR      	|                                    	| #f00  	|
| GND      	| 13         	|            	|     | OE       	|                                    	| #000  	|
| GND      	| 8          	| 8          	| 8   | GND      	|                                    	| #000  	|
| GND      	|            	| 15         	|     | CE       	|                                    	| #000  	|
| GND      	|            	|            	| 6   | CLK INH 	|                                    	| #000  	|
| 00 Strap 	| 🚫          | 🚫       	  |     | ESP       | Boot Mode                          	|       	|
| 01 ✅     |             |             |     | 19 BR8   	|                                    	| #0ff  	|
| 02 ✅    	|            	|             |     | 18 MK9   	|                                    	| #0ff  	|
| 03 Strap	| 🚫         	| 🚫       	  |     | ESP      	| JTAG Debug                         	|       	|
| 04      	|            	|            	|     |          	| Weak pull up on reset             	|       	|
| 05 ✅    	| 14 DS      	|            	|     |          	|                                    	|       	|
| 06 ✅    	| 12 STCP    	| 1 (PL)     	|     | SH/LD    	|                                    	| #f70  	|
| 07 ✅    	| 11 SHCP    	|            	|     |          	|                                    	|       	|
| 08 ✅    	|           	|            	|     |          	|                                    	|       	|
| 09 ✅    	|            	|            	|     |          	|                                    	|       	|
| 10 ✅   	|            	|            	|     | 20 BR9  	|                                    	|       	|
| 11 ✅    	|            	|            	|     |          	|                                    	|       	|
| 12 ✅    	|            	|            	|     |          	|                                    	|       	|
| 13 ✅    	|            	|            	|     | 17 MK8   	|                                    	|       	|
| 14 ✅    	|            	|            	|     |          	|                                    	|       	|
| 15 ✅    	|            	|            	| 13B | QH      	|                                    	| #070    |
| 16 ✅    	|            	|            	| 15  | SH/LD   	|                                    	| #00f    |
| 17 ✅    	|            	|            	| 7   | CLK      	|                                   	| #202    |
| 18 ✅    	|            	|            	| 13M | QH       	|                                       | #555    |
| 19 🚫🚫 	| 🚫         	| 🚫         	|     | ESP      	| USB Data                           	|       	|
| 20 🚫🚫 	| 🚫         	| 🚫         	|     | ESP      	| USB Data                           	|       	|
| 26       	| 🚫         	| 🚫         	|     | ESP      	| SPI FLASH                          	|       	|
| 27       	| 🚫         	| 🚫         	|     | ESP      	| SPI FLASH                          	|       	|
| 28       	| 🚫         	| 🚫         	|     | ESP      	| SPI FLASH                          	|       	|
| 29       	| 🚫         	| 🚫         	|     | ESP      	| SPI FLASH                          	|       	|
| 30       	| 🚫         	| 🚫         	|     | ESP      	| SPI FLASH                          	|       	|
| 31       	| 🚫         	| 🚫         	|     | ESP      	| SPI FLASH                          	|       	|
| 32       	| 🚫         	| 🚫         	|     | ESP      	| SPI FLASH                          	|       	|
| 35 🚫🚫 	| 🚫         	| 🚫         	|     | ESP      	| Internal communication SPI - PSRAM 	|       	|
| 36 🚫🚫 	| 🚫         	| 🚫         	|     | ESP      	| Internal communication SPI - PSRAM 	|       	|
| 37 🚫🚫  	| 🚫         	| 🚫         	|     | ESP      	| Internal communication SPI - PSRAM 	|       	|
| 38       	|            	|           	|     |          	| (See USB OTG Boot Mode)           	|       	|
| 39       	| 🚫         	| 🚫         	|     | ESP      	| JTAG Debug                         	|       	|
| 40       	| 🚫         	| 🚫         	|     | ESP      	| JTAG Debug                         	|       	|
| 41       	| 🚫         	| 🚫         	|     | ESP      	| JTAG Debug                         	|       	|
| 42       	| 🚫         	| 🚫         	|     | ESP      	| JTAG Debug                         	|       	|
| 43 🚫🚫  	| 🚫         	| 🚫         	|     | ESP      	| USB Serial/UART                    	|       	|
| 44 🚫🚫  	| 🚫         	| 🚫         	|     | ESP      	| USB Serial/UART                    	|       	|
| 45       	| 🚫         	| 🚫        	|     | ESP      	| Internal Flash Voltage             	|       	|
| 46       	| 🚫         	| 🚫         	|     | ESP      	| Boot Mode/ROM msg                  	|       	|
| 47       	|            	|            	|     |          	|                                    	|       	|
| 48 ✅    	|            	|            	|     |          	|                                    	|       	|

### Pin names 74hc595

| Pin	   | Description |
|:------:| ----------- |
| DS	   | Serial input |
| SHCP	 | Serial clock |
| STCP	 | Storage (latch) pin |
| OE	   | Output enable, active low. Connect to GND if not used. |
| Q0…Q7  | Parallel output |
| Q7S	   | Serial output* |
| MR	   | Reset (clear), active low. Connect to VCC if not used |
| GND	   | Ground |
| VCC	   | Supply voltage |

* Use the Q7S to chain multiple 74HC595 units together. Connect Q7S to the DS pin of the next 74HC595 chip in chain.

### Pin names 74HC165

CP (Clock Pulse ou Shift Clock): É a entrada de clock (sino) do componente. A cada pulso de subida (transição de nível baixo para nível alto), o chip desloca o próximo bit de dado para fora através do pino Q7.

PL (Parallel Load ou Shift/Load): É o pino que alterna o estado do chip.
 - Quando está em nível BAIXO (0V), o chip lê e captura simultaneamente os estados das 8 entradas paralelas (D0 a D7).
 - Quando está em nível ALTO (5V), o chip entra em modo de leitura serial, permitindo que você envie pulsos no CP para ler os dados um por um.

Q7 (ou QH): É a saída serial de dados. É por este pino que os bits (referentes às 8 entradas) saem sequencialmente, bit a bit, a cada pulso de clock.

| Pin	   | Description |
|:--------:| ----------- |
| D0…D7    |	Parallel input |
| PL	   | Parallel load (active low) |
| CP	   | Serial clock |
| CE	   | Clock enable (active low) |
| Q7	   | Serial output |
| Q7_N     | Inverted serial output (usually not used) |
| DS	   | Serial input* |
| GND	   | Ground |
| VCC	   | Supply voltage |

* Use the DS to daisy-chain multiple 74HC165 units together. Connect DS to the Q7 pin of the previous 74HC165 chip in the chain. You can leave DS disconnected if you don't chain or for the first chip in the chain.

| ESP GPIO | Function     | Connected To              |     |     |
| -------- | ------------ | ------------------------- | --- | --- |
| GPIO  8  | SR_DATA_OUT  | 74HC595 SER               |     |     |
| GPIO 17  | SR_CLOCK     | 74HC595 SRCLK, 74HC166 CP |     |     |
| GPIO 15  | SR_LATCH     | 74HC595 RCLK              |     |     |
| GPIO  6  | SR_LOAD      | (15) 74HC166 /PL          |     |     |
| GPIO 18  | QH do 166 MK | 74HC166 MK QH             |     |     |
| GPIO  7  | QH do 166 BR | 74HC166 BR QH             |     |     |

---

## 74HC595 (TX Scanner)

Only one output shall be driven LOW at a time.

All remaining outputs remain HIGH.

| 74HC595 Output | Matrix Line |
| -------------- | ----------- |
| Q0             | T0          |
| Q1             | T1          |
| Q2             | T2          |
| Q3             | T3          |
| Q4             | T4          |
| Q5             | T5          |
| Q6             | T6          |
| Q7             | T7          |

---

## 74HC166 #1

| Input | Signal |
| ----- | ------ |
| D0    | MK0    |
| D1    | MK1    |
| D2    | MK2    |
| D3    | MK3    |
| D4    | MK4    |
| D5    | MK5    |
| D6    | MK6    |
| D7    | MK7    |

---

## 74HC166 #2

| Input | Signal |
| ----- | ------ |
| D0    | MK8    |
| D1    | MK9    |
| D2    | BR0    |
| D3    | BR1    |
| D4    | BR2    |
| D5    | BR3    |
| D6    | BR4    |
| D7    | BR5    |

---

## 74HC166 #3

| Input | Signal |
| ----- | ------ |
| D0    | BR6    |
| D1    | BR7    |
| D2    | BR8    |
| D3    | BR9    |
| D4    | NC     |
| D5    | NC     |
| D6    | NC     |
| D7    | NC     |

---

## Keybed Connectors

### Bass Header

| Pin | Signal |
| --- | ------ |
| 1   | MK0    |
| 2   | T0     |
| 3   | BR0    |
| 4   | T1     |
| 5   | MK1    |
| 6   | T2     |
| 7   | BR1    |
| 8   | T3     |
| 9   | MK2    |
| 10  | T4     |
| 11  | BR2    |
| 12  | T5     |
| 13  | MK3    |
| 14  | T6     |
| 15  | BR3    |
| 16  | T7     |

### Treble Header

| Pin | Signal |
| --- | ------ |
| 1   | MK4    |
| 2   | T0     |
| 3   | BR4    |
| 4   | T1     |
| 5   | MK5    |
| 6   | T2     |
| 7   | BR5    |
| 8   | T3     |
| 9   | MK6    |
| 10  | T4     |
| 11  | BR6    |
| 12  | T5     |
| 13  | MK7    |
| 14  | T6     |
| 15  | BR7    |
| 16  | T7     |
| 17  | MK8    |
| 18  | MK9    |
| 19  | BR8    |
| 20  | BR9    | 

---

## Matrix Dimensions

TX_COUNT = 8
MKBR_COUNT = 10
Matrix Positions = 80
Used Keys = 76
Unused Positions = 4
The firmware shall use a lookup table to map matrix positions to MIDI note numbers.


# 20 Pin Headers

```
              ____
+-------------    -------------+
| 1  3  5  7  9 11 13 15 17 19 |
| 2  4  6  8 10 12 14 16 18 20 |
+------------------------------+
```

## Bass

|     |    |  2  |   4 |  6  |   8 | 10  |  12 | 14  |  16 |
|:---:|:--:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| MK0 |  1 | E0  |  F0 | F#0 | G0  | G#0 | A1  | A#1 | B1  |
| BR0 |  3 | E0  |  F0 | F#0 | G0  | G#0 | A1  | A#1 | B1  |
| MK1 |  5 | C1  | C#1 | D1  | D#1 | E1  | F1  | F#1 | G1  |
| BR1 |  7 | C1  | C#1 | D1  | D#1 | E1  | F1  | F#1 | G1  |
| MK2 |  9 | G#1 | A2  | A#2 | B2  | C2  | C#2 | D2  | D#2 |
| BR2 | 11 | G#1 | A2  | A#2 | B2  | C2  | C#2 | D2  | D#2 |
| MK3 | 13 | E2  |  F2 | F#2 | G2  | G#2 | A3  | A#3 | B3  |
| BR3 | 15 | E2  |  F2 | F#2 | G2  | G#2 | A3  | A#3 | B3  |

## Treble

|     |    |  2  |   4 |  6  |   8 | 10  |  12 | 14  |  16 |
|:---:|:--:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| MK4 |  1 |  C3 | C#3 |  D3 | D#3 |  E3 |  F3 | F#3 |  G3 |
| BR4 |  3 |  C3 | C#3 |  D3 | D#3 |  E3 |  F3 | F#3 |  G3 |
| MK5 |  5 | G#3 | A4  | A#4 | B4  | C4  | C#4 | D4  | D#4 |
| BR5 |  7 | G#3 | A4  | A#4 | B4  | C4  | C#4 | D4  | D#4 |
| MK6 |  9 | E4  |  F4 | F#4 | G4  | G#4 | A5  | A#5 | B5  |
| BR6 | 11 | E4  |  F4 | F#4 | G4  | G#4 | A5  | A#5 | B5  |
| MK7 | 13 |  C5 | C#5 |  D5 | D#5 |  E5 |  F5 | F#5 |  G5 |
| BR7 | 15 |  C5 | C#5 |  D5 | D#5 |  E5 |  F5 | F#5 |  G5 |
| MK8 | 17 | G#5 |  A6 | A#6 |  B6 |  C6 | C#6 |  D6 | D#6 |
| BR8 | 19 | G#5 |  A6 | A#6 |  B6 |  C6 | C#6 |  D6 | D#6 |
| MK9 | 18 |  E6 |  F6 | F#6 |  G6 |
| BR9 | 20 |  E6 |  F6 | F#6 |  G6 |
