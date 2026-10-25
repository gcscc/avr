// main.c - Controlador MIDI: keybed 76 teclas + pedais + rodas + controles
// ATmega2560 @ 16 MHz, avr-gcc puro (sem Arduino).
// Saida unica: UART0 (a mesma porta USB da placa), MIDI cru ou debug em texto.
//
// Compilar:  avr-gcc -mmcu=atmega2560 -O2 -Wall -o main.elf main.c -lm
//            debug em texto: acrescente -DSERIAL_MODE=2  (ou edite o padrao abaixo)
//
// Ligacoes:
//   T0..T7            -> PA0..PA7 (pinos 22..29), pull-up externo 4,7k para 5V
//   Linhas 0..7       -> PC0..PC7   (par = 1o contato, impar = 2o contato)
//   Linhas 8..15      -> PL0..PL7
//   Linhas 16..19     -> PK0..PK3
//   Modulation  CC1   -> PF0 (A0)
//   Pitch bend        -> PF1 (A1)
//   Expression  CC11  -> PF2 (A2)   pedal analogico, pode ficar desconectado
//   Volume      CC7   -> PF3 (A3)
//   Damper      CC64  -> PE4 (pino 2)  chave para GND (pull-up interno)
//   Sostenuto   CC66  -> PE5 (pino 3)
//   Soft        CC67  -> PG5 (pino 4)

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <stdint.h>

/* ============================ MODO DA UART0 (porta USB) ============================ */
/* A UART0 serve ou para MIDI cru ou para texto de debug; os dois no mesmo fluxo se corromperiam. */
#define SERIAL_MIDI   1
#define SERIAL_DEBUG  2
#ifndef SERIAL_MODE
#define SERIAL_MODE   SERIAL_MIDI      // padrao: MIDI. Nao precisa de nenhuma flag de compilacao.
#endif
#if SERIAL_MODE != SERIAL_MIDI && SERIAL_MODE != SERIAL_DEBUG
#error "SERIAL_MODE deve ser 1 (MIDI) ou 2 (debug em texto)"
#endif
#define USB_MIDI  (SERIAL_MODE == SERIAL_MIDI)
#define DBG       (SERIAL_MODE == SERIAL_DEBUG)

#ifndef DEBUG_ADC
#define DEBUG_ADC 0                    // 1 (so com SERIAL_MODE=2): imprime o valor bruto 0..1023 dos analogicos
#endif

/* ============================ CONFIGURACAO ============================ */
#ifndef USB_BAUD
#define USB_BAUD        115200UL       // use 31250UL ao migrar para o 16U2 com MocoLUFA
#endif
#define USB_RUNNING_STATUS 0           // 0 = status em toda mensagem (compativel com qualquer ponte)
#define MIDI_CH         0              // 0 = canal 1
#define BASE_NOTE       28             // E0
#define SETTLE_CYC      16             // ciclos de espera por linha (16 = 1 us)

#define DT_MIN_US       4000           // dt <= isso  -> velocity 127
#define DT_MAX_US       66000          // dt >= isso  -> velocity 1
#define SIG_K           4.0f           // inclinacao da sigmoide
#define SIG_MID         0.5f           // centro da S na escala log (0..1)
#define VT_N            ((DT_MAX_US / 32) + 2)   // entradas da tabela (passo de 32 us)
#define ORPHAN_VEL      0              // >0: toca com essa velocity se so o 2o contato fechar

#define T_DEBOUNCE      1250           // ticks de 4 us (5 ms): repique do 2o contato ao abrir
#define T_RETRIG        1000           // 4 ms: retrigger minimo sem soltar a tecla
#define T_STALE         7500           // 30 ms: 1o contato antigo, rearma

/* canais do ADC (0..7 = PF0..PF7). 8..15 (PK) conflitam com as linhas 16..19 da matriz. */
#define ADC_MOD         0              // PF0
#define ADC_PB          1              // PF1
#define ADC_EXPR        2              // PF2
#define ADC_VOL         3              // PF3
#if (ADC_MOD > 7) || (ADC_PB > 7) || (ADC_EXPR > 7) || (ADC_VOL > 7)
#error "Canais ADC 8..15 (PK0..PK7) conflitam com as linhas 16..19 da matriz. Use ADC0..ADC7 (PF0..PF7)."
#endif

/* calibracao: valores brutos do ADC (0..1023). Meca com SERIAL_MODE=2 e DEBUG_ADC=1 */
#define PB_RAW_MIN      100            // cursor todo para baixo
#define PB_RAW_MAX      920            // cursor todo para cima (o centro e medido no boot)
#define PB_DEAD         6              // zona morta em volta do centro (contagens brutas)
#define PB_INVERT       0              // 1 inverte o sentido
#define MOD_RAW_MIN     30
#define MOD_RAW_MAX     1000
#define EXPR_RAW_MIN    30
#define EXPR_RAW_MAX    1000
#define VOL_RAW_MIN     30
#define VOL_RAW_MAX     1000
#define ADC_HYST        24             // histerese em 1/16 de contagem (24 = 1,5 contagens)

/* pedal de expressao: deteccao de conexao pelo pull-up interno */
#define EXPR_PROBE      1              // 0 = considera sempre conectado
#define EXPR_CONN_THRESH 1000          // leitura abaixo disso (com pull-up) = ha um pedal ligado
#define EXPR_CONN_COUNT  16            // leituras consecutivas para confirmar a conexao
/* ====================================================================== */

enum { S_IDLE, S_ARMED, S_DOWN, S_REARM };

static uint8_t  prev[20];
static uint8_t  st[80];
static uint16_t t0[80];
static uint8_t  velT[VT_N];      // indice = dt / 32 us

/* ============================ UART0 (USB) ============================ */
static volatile uint8_t ubuf[256];                     // indices de 8 bits: a volta e automatica
static volatile uint8_t uh, ut;

ISR(USART0_UDRE_vect) {
    uint8_t t = ut;
    UDR0 = ubuf[t++];
    ut = t;
    if (t == uh) UCSR0B &= ~(1 << UDRIE0);
}

#if DBG
static void dputc(char c) {
    uint8_t nx = uh + 1;
    if (nx != ut) { ubuf[uh] = c; uh = nx; }
}
static void dputs(const char *s) { while (*s) dputc(*s++); }
static void dputu(uint16_t v) {
    char b[5]; uint8_t n = 0;
    do { b[n++] = '0' + v % 10; v /= 10; } while (v);
    while (n) dputc(b[--n]);
}
static void dputhex(uint8_t v) {
    const char *h = "0123456789ABCDEF";
    dputc(h[v >> 4]); dputc(h[v & 15]);
}
static inline void dkick(void) { UCSR0B |= (1 << UDRIE0); }
#endif

/* ============================ saida MIDI ============================ */
#if USB_MIDI
static uint8_t ulast;

// Mensagem de 3 bytes. Buffer cheio: descarta, nunca trava a varredura.
static void midi3(uint8_t s, uint8_t a, uint8_t b) {
    uint8_t h = uh, t = ut;
    if (h == t) ulast = 0;                             // fila vazia: reenvia o status
    uint8_t rs = USB_RUNNING_STATUS && (s == ulast);
    uint8_t need = rs ? 2 : 3;
    if ((uint8_t)(t - h - 1) < need) return;
    if (!rs) { ubuf[h++] = s; ulast = s; }
    ubuf[h++] = a;
    ubuf[h++] = b;
    uh = h;
    UCSR0B |= (1 << UDRIE0);
}
#else
// Modo debug: mensagens que nao sao nota aparecem como "tx XX XX XX" (as notas saem em note_on)
static void midi3(uint8_t s, uint8_t a, uint8_t b) {
    if ((s & 0xE0) == 0x80) return;
    dputs("tx "); dputhex(s); dputc(' '); dputhex(a); dputc(' '); dputhex(b);
    dputs("\r\n"); dkick();
}
#endif

static void note_on(uint8_t k, uint8_t vel, uint16_t dt) {
    midi3(0x90 | MIDI_CH, BASE_NOTE + k, vel);
#if DBG
    dputs("ON  n="); dputu(BASE_NOTE + k);
    dputs(" dt="); dputu(dt > 16383 ? 65535u : (uint16_t)(dt << 2));
    dputs("us v="); dputu(vel); dputs("\r\n"); dkick();
#else
    (void)dt;
#endif
}
static void note_off(uint8_t k) {
    midi3(0x80 | MIDI_CH, BASE_NOTE + k, 64);
}
#if DBG && !ORPHAN_VEL
static void orphan(uint8_t k) {
    dputs("?   n="); dputu(BASE_NOTE + k); dputs(" sem 1o contato\r\n"); dkick();
}
#endif

/* ============================ velocity ============================ */
// Sigmoide na escala logaritmica do tempo, normalizada: DT_MIN -> 127, DT_MAX -> 1.
static uint8_t curve(float us) {
    if (us <= (float)DT_MIN_US) return 127;
    if (us >= (float)DT_MAX_US) return 1;
    float x = logf(us / (float)DT_MIN_US) / logf((float)DT_MAX_US / (float)DT_MIN_US);
    float s  = 1.0f / (1.0f + expf(-SIG_K * (x - SIG_MID)));
    float s0 = 1.0f / (1.0f + expf( SIG_K * SIG_MID));
    float s1 = 1.0f / (1.0f + expf(-SIG_K * (1.0f - SIG_MID)));
    float y = (s - s0) / (s1 - s0);
    if (y < 0.0f) y = 0.0f;
    if (y > 1.0f) y = 1.0f;
    return (uint8_t)(127.5f - y * 126.0f);             // 127..1
}
static inline uint8_t velocity(uint16_t dt) {          // dt em ticks de 4 us
    uint16_t i = dt >> 3;                              // 32 us por entrada
    if (i >= VT_N) return 1;
    return velT[i];
}

/* ============================ maquina de estados por tecla ============================ */
static void __attribute__((noinline)) handle(uint8_t n, uint8_t cur, uint8_t diff, uint16_t now) {
    uint8_t k0     = (n >> 1) << 3;
    uint8_t second = n & 1;                            // linha impar = 2o contato
    uint8_t other  = prev[n ^ 1];

    for (uint8_t t = 0, bit = 1; diff; t++, bit <<= 1) {
        if (!(diff & bit)) continue;
        diff &= ~bit;
        uint8_t k = k0 + t;
        if (k >= 76) continue;                         // grupo 9 so tem 4 teclas
        uint16_t dt = now - t0[k];
        uint8_t  s  = st[k];

        if (cur & bit) {                               // fechou
            if (!second) {
                if (s == S_IDLE || s == S_REARM || (s == S_ARMED && dt > T_STALE)) {
                    st[k] = S_ARMED; t0[k] = now;
                }
            } else if (s == S_ARMED || (s == S_REARM && dt >= T_RETRIG)) {
                note_on(k, velocity(dt), dt);
                st[k] = S_DOWN; t0[k] = now;
            } else if (s == S_IDLE) {
#if ORPHAN_VEL
                note_on(k, ORPHAN_VEL, 0);
                st[k] = S_DOWN; t0[k] = now;
#elif DBG
                orphan(k);
#endif
            }
        } else {                                       // abriu
            if (s == S_DOWN) {
                if (second && dt < T_DEBOUNCE) continue;   // repique
                note_off(k);
                if (second && (other & bit)) { st[k] = S_REARM; t0[k] = now; }
                else st[k] = S_IDLE;
            } else if (!second && s == S_REARM) {
                st[k] = S_IDLE;
            }
        }
    }
}

static inline void check(uint8_t n, uint8_t cur) {
    uint8_t diff = cur ^ prev[n];
    if (__builtin_expect(diff, 0)) {
        uint16_t now = TCNT1;
        prev[n] = cur;
        handle(n, cur, diff, now);
    }
}
static inline void initLine(uint8_t n, uint8_t cur) { prev[n] = cur; }

/* ============================ pedais (chaves) ============================ */
/* Polaridade automatica: o nivel lido no boot e o estado "solto". Pedal desconectado le
   nivel alto (pull-up interno), vira "solto" e nunca gera mensagem. */
static const uint8_t pedCC[3] = {64, 66, 67};          // bit0 PE4 damper, bit1 PE5 sostenuto, bit2 PG5 soft
static uint8_t  prest;                                 // nivel de repouso lido no boot
static uint8_t  pst, pcand;                            // pressionados (debounceado) / candidato
static uint16_t ptime;

static inline uint8_t ped_raw(void) {                  // 1 = nivel alto no pino
    uint8_t e = PINE, g = PING;
    return ((e >> 4) & 0x03) | (((g >> 5) & 0x01) << 2);
}
static inline uint8_t ped_pressed(void) { return (ped_raw() ^ prest) & 0x07; }

static void pedal_poll(uint16_t now) {
    if ((uint16_t)(now - ptime) < 500) return;         // a cada 2 ms
    ptime = now;
    uint8_t p = ped_pressed();
    if (p != pcand) { pcand = p; return; }             // debounce: 2 leituras iguais
    uint8_t d = p ^ pst;
    if (!d) return;
    pst = p;
    for (uint8_t i = 0; i < 3; i++) {
        uint8_t bit = 1 << i;
        if (d & bit) midi3(0xB0 | MIDI_CH, pedCC[i], (p & bit) ? 127 : 0);
    }
}

/* ============================ analogicos ============================ */
// Valores internos em 1/16 de contagem do ADC.
enum { C_MOD, C_PB, C_EXPR, C_VOL, NADC };

static const uint8_t adcCh[NADC] = { ADC_MOD, ADC_PB, ADC_EXPR, ADC_VOL };
static const uint8_t ccNum[NADC] = { 1, 0, 11, 7 };    // C_PB e pitch bend (nao usa CC)
static const int16_t rmin[NADC]  = { MOD_RAW_MIN, 0, EXPR_RAW_MIN, VOL_RAW_MIN };
static const int16_t rmax[NADC]  = { MOD_RAW_MAX, 0, EXPR_RAW_MAX, VOL_RAW_MAX };

static int16_t  af[NADC];            // filtrado
static int16_t  ac[NADC];            // "commitado": muda so quando passa da histerese
static uint8_t  aidx;
static int16_t  cmin[NADC];
static uint32_t cscale[NADC];
static uint8_t  clast[NADC];

static int16_t  pb_c16, pb_up_span, pb_dn_span;
static uint32_t pb_up_scale, pb_dn_scale;
static int16_t  pb_last = 8192;

static void pb_emit(void) {
    int16_t d = ac[C_PB] - pb_c16;
    int16_t v = 8192;
    if (d > PB_DEAD * 16) {
        d -= PB_DEAD * 16;
        if (d > pb_up_span) d = pb_up_span;
        v = 8192 + (int16_t)(((uint32_t)d * pb_up_scale) >> 16);
    } else if (d < -(PB_DEAD * 16)) {
        d = -d - PB_DEAD * 16;
        if (d > pb_dn_span) d = pb_dn_span;
        v = 8192 - (int16_t)(((uint32_t)d * pb_dn_scale) >> 16);
    }
    if (v < 0) v = 0;
    if (v > 16383) v = 16383;
#if PB_INVERT
    v = 16383 - v;
#endif
    if (v != pb_last) {
        pb_last = v;
        midi3(0xE0 | MIDI_CH, v & 0x7F, (v >> 7) & 0x7F);
    }
}

static void cc_emit(uint8_t i) {
    int16_t d = ac[i] - cmin[i];
    int32_t v = (d <= 0) ? 0 : (int32_t)(((uint32_t)d * cscale[i]) >> 16);
    if (v > 127) v = 127;
    if ((uint8_t)v != clast[i]) {
        clast[i] = (uint8_t)v;
        midi3(0xB0 | MIDI_CH, ccNum[i], (uint8_t)v);
    }
}

static void adc_process(uint8_t i, int16_t raw16) {
    int16_t f = af[i] + ((raw16 - af[i]) >> 2);        // filtro passa-baixa
    af[i] = f;
    int16_t dd = f - ac[i];
    if (dd < 0) dd = -dd;
    if (dd >= ADC_HYST) {
        ac[i] = f;
#if DBG && DEBUG_ADC
        dputs("A"); dputu(i); dputs("="); dputu((uint16_t)(f >> 4)); dputs("\r\n"); dkick();
#endif
        if (i == C_PB) pb_emit(); else cc_emit(i);
    }
}

#if EXPR_PROBE
/* Pedal de expressao desconectado: o pino fica com o pull-up interno ligado e le ~1023.
   Ao ligar um pedal a leitura cai; so depois de EXPR_CONN_COUNT leituras baixas seguidas ele e
   considerado conectado, o pull-up sai, e a posicao atual vira a base (sem enviar mensagem). */
enum { EX_OFF, EX_SETTLE, EX_ON };
static uint8_t expr_state, expr_cnt;

static void expr_track(uint16_t r) {
    if (expr_state == EX_OFF) {
        if (r < EXPR_CONN_THRESH) {
            if (++expr_cnt >= EXPR_CONN_COUNT) {
                PORTF &= ~(1 << ADC_EXPR);             // solta o pull-up
                expr_cnt = 0;
                expr_state = EX_SETTLE;
            }
        } else {
            expr_cnt = 0;
        }
    } else {                                           // EX_SETTLE: ignora 4 leituras e fixa a base
        if (++expr_cnt >= 4) {
            af[C_EXPR] = ac[C_EXPR] = (int16_t)(r << 4);
            expr_state = EX_ON;
#if DBG
            dputs("expressao conectada\r\n"); dkick();
#endif
        }
    }
}
#endif

static void adc_poll(void) {
    if (ADCSRA & (1 << ADSC)) return;                  // conversao em andamento
    uint8_t i = aidx;
    uint16_t r = ADC;                                  // resultado do canal i
#if EXPR_PROBE
    if (i == C_EXPR && expr_state != EX_ON) {
        expr_track(r);
    } else
#endif
    {
        adc_process(i, (int16_t)(r << 4));
    }
    i++; if (i == NADC) i = 0;
    aidx = i;
    ADMUX = (1 << REFS0) | adcCh[i];
    ADCSRA |= (1 << ADSC);
}

static uint16_t adc_blocking(uint8_t ch) {
    ADMUX = (1 << REFS0) | ch;
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {}
    return ADC;
}

static void analog_init(void) {
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);   // clk/128
    DIDR0 = (1 << ADC_MOD) | (1 << ADC_PB) | (1 << ADC_EXPR) | (1 << ADC_VOL);
    (void)adc_blocking(ADC_PB);                                           // descarta a 1a

    // centro do pitch bend: media de 32 leituras com a roda solta
    uint16_t sum = 0;
    for (uint8_t i = 0; i < 32; i++) sum += adc_blocking(ADC_PB);
    pb_c16 = (int16_t)(sum >> 1);                                         // = media * 16
    af[C_PB] = ac[C_PB] = pb_c16;

    int16_t up = PB_RAW_MAX * 16 - pb_c16 - PB_DEAD * 16;
    int16_t dn = pb_c16 - PB_RAW_MIN * 16 - PB_DEAD * 16;
    if (up < 1) up = 1;
    if (dn < 1) dn = 1;
    pb_up_span = up; pb_dn_span = dn;
    pb_up_scale = (8191UL << 16) / (uint16_t)up;
    pb_dn_scale = (8191UL << 16) / (uint16_t)dn;

    // demais canais: a posicao do boot vira a base, nada e enviado
    for (uint8_t i = 0; i < NADC; i++) {
        if (i == C_PB) continue;
        uint16_t r = adc_blocking(adcCh[i]);
        af[i] = ac[i] = (int16_t)(r << 4);
        cmin[i] = rmin[i] * 16;
        int16_t span = (rmax[i] - rmin[i]) * 16;
        if (span < 16) span = 16;
        cscale[i] = (127UL << 16) / (uint16_t)span;
        clast[i] = 0xFF;
    }

#if EXPR_PROBE
    PORTF |= (1 << ADC_EXPR);                                             // pull-up ate haver pedal
    expr_state = EX_OFF;
#endif

    aidx = 0;
    ADMUX = (1 << REFS0) | adcCh[0];
    ADCSRA |= (1 << ADSC);
}

static void __attribute__((noinline)) periph(void) {
    pedal_poll(TCNT1);
    adc_poll();
}

/* ============================ varredura ============================ */
#define SCAN_PORT(PORT, NB, L0, ACT)                     \
    do {                                                 \
        uint8_t m = 0xFE;                                \
        for (uint8_t i = 0; i < (NB); i++) {             \
            PORT = m;                                    \
            __builtin_avr_delay_cycles(SETTLE_CYC);      \
            ACT((L0) + i, ~PINA);                        \
            m = (uint8_t)((m << 1) | 1);                 \
        }                                                \
        PORT = 0xFF;                                     \
    } while (0)

#define SCAN_ALL(ACT)                                    \
    do {                                                 \
        SCAN_PORT(PORTC, 8, 0,  ACT);                    \
        SCAN_PORT(PORTL, 8, 8,  ACT);                    \
        SCAN_PORT(PORTK, 4, 16, ACT);                    \
    } while (0)

int main(void) {
    // matriz
    PORTC = 0xFF; PORTL = 0xFF; PORTK = 0xFF;
    DDRC = 0xFF; DDRL = 0xFF; DDRK = 0x0F;
    DDRA = 0x00; PORTA = 0x00;                         // T: pull-up externo

    // pedais: entradas com pull-up interno (so os bits usados; PE0/PE1 sao a UART0)
    DDRE &= ~0x30; PORTE |= 0x30;                      // PE4, PE5
    DDRG &= ~0x20; PORTG |= 0x20;                      // PG5

    // Timer1: 4 us por tick
    TCCR1B = (1 << CS11) | (1 << CS10);

    // UART0 (USB): 8N1, U2X0
    UBRR0 = (uint16_t)((F_CPU / (8UL * USB_BAUD)) - 1);    // 115200 -> 16 ; 31250 -> 63
    UCSR0A = (1 << U2X0);
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    for (uint16_t i = 0; i < VT_N; i++) velT[i] = curve(i * 32.0f + 16.0f);

    analog_init();

    // Pedais: o estado de agora e o "solto". As tabelas e o ADC acima levam centenas de ms,
    // tempo de sobra para os pull-ups estabilizarem antes desta leitura.
    prest = ped_raw();
    pst = pcand = 0;
    ptime = TCNT1;

    sei();

    uint16_t a = TCNT1;                                // mede a varredura
    for (uint8_t i = 0; i < 100; i++) SCAN_ALL(initLine);
    uint16_t b = TCNT1;

#if DBG
    dputs("pronto, varredura = "); dputu((b - a) / 25); dputs(" us\r\n");
    dputs("pitch bend centro = "); dputu((uint16_t)(pb_c16 >> 4)); dputs("\r\n");
    dputs("pedais repouso = 0x"); dputhex(prest & 0x07); dputs(" (bit0 damper, bit1 sostenuto, bit2 soft)\r\n");
    {
        uint8_t seen[16] = {0}, cnt = 0, maxjump = 0;
        for (uint16_t i = 0; i < VT_N; i++) {
            uint8_t v = velT[i];
            if (!(seen[v >> 3] & (1 << (v & 7)))) { seen[v >> 3] |= (1 << (v & 7)); cnt++; }
            if (i) {
                uint8_t j = (velT[i - 1] > v) ? velT[i - 1] - v : v - velT[i - 1];
                if (j > maxjump) maxjump = j;
            }
        }
        dputs("velocity: niveis na tabela = "); dputu(cnt);
        dputs(", maior salto entre entradas = "); dputu(maxjump); dputs("\r\n");
    }
    for (uint8_t n = 0; n < 20; n++) {
        uint8_t v = prev[n];
        if (n >= 18) v &= 0x0F;
        if (v) { dputs("preso linha "); dputu(n); dputs(" T=0x"); dputhex(v); dputs("\r\n"); }
    }
    dkick();
#else
    (void)a; (void)b;
#endif

    for (;;) {
        SCAN_ALL(check);
        periph();
    }
}
