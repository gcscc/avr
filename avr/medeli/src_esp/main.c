#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define NUM_KEYS 76
#define NUM_STROBES 10 // 10 pares de linhas (MK e BR)

// Tabela de conversão delta_t (ticks de 200us) -> Velocity (1 a 127)
const uint8_t PROGMEM velocity_lut[128] = {
    127, 127, 127, 125, 120, 115, 110, 105, 100, 95, 90, 85, 80, 75, 70, 65,
    60,  55,  50,  45,  40,  35,  30,  28,  26, 24, 22, 20, 18, 16, 14, 12,
    10,   9,   8,   7,   6,   5,   4,   3,   2,  1,  1,  1,  1,  1,  1,  1,
    // Preenchido até 128 posições
};

typedef struct {
    uint16_t t1_tick;
    uint8_t  state; // 0: Repouso, 1: MK fechado (Aguardando BR), 2: Note On enviado
} KeyState;

volatile KeyState keys[NUM_KEYS];
volatile uint16_t global_ticks = 0;
volatile uint8_t current_strobe = 0;

void uart_init(void) {
    // Para F_CPU = 16 MHz, BAUD = 115200 e U2X0 = 1:
    // UBRR = (16000000 / (8 * 115200)) - 1 = 16.33 -> 16
    UBRR0H = 0;
    UBRR0L = 16;

    UCSR0A |= (1 << U2X0);                  // Ativa modo Double Speed
    UCSR0B = (1 << TXEN0);                  // Habilita Transmissão (TX)
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Formato 8N1
}

// void uart_init(unsigned int ubrr) {
//     UCSR0A |= (1 << U2X0); // Double Speed
//     UBRR0H = (unsigned char)(ubrr >> 8);
//     UBRR0L = (unsigned char)ubrr;
//     UCSR0B = (1 << TXEN0); // Habilita apenas TX para envio MIDI
//     UCSR0C = (1 << UCSZ00) | (1 << UCSZ01); // 8N1
// }

void midi_send(uint8_t status, uint8_t data1, uint8_t data2) {
    while (!(UCSR0A & (1 << UDRE0))); UDR0 = status;
    while (!(UCSR0A & (1 << UDRE0))); UDR0 = data1;
    while (!(UCSR0A & (1 << UDRE0))); UDR0 = data2;
}

// Interrupção do Timer1: Roda a cada 200 us (frequência de 5 kHz)
ISR(TIMER1_COMPA_vect) {
    global_ticks++;

    // 1. Desativa todas as saídas de varredura (Coloca em HIGH)
    PORTL = 0xFF;
    PORTC = 0xFF;

    // 2. Seleciona qual porta/pino deve ir para LOW (MK) e (BR)
    // Usamos PORTL para MK (Make) e PORTC para BR (Break)
    uint8_t mk_pin = current_strobe; 

    // Ativa a linha MK atual colocando em LOW
    PORTL &= ~(1 << mk_pin);

    // CRITICAL FIX: Aguarda descarga capacitiva da trilha
    asm volatile(
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
    );

    // Leitura direta do Bus A (MK) -> 0 significa chave fechada
    uint8_t mk_bus = PINA;

    // Agora desativa MK e ativa a linha BR (Break) correspondente
    PORTL |= (1 << mk_pin);
    PORTC &= ~(1 << mk_pin);

    asm volatile(
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
    );

    // Leitura do Bus B (BR)
    uint8_t br_bus = PINA;

    // Processa os 8 bits de notas associados a essa linha de varredura
    for (uint8_t col = 0; col < 8; col++) {
        uint8_t key_idx = (current_strobe * 8) + col;
        if (key_idx >= NUM_KEYS) break;

        // Bit = 0 indica contato fechado
        uint8_t mk_closed = !(mk_bus & (1 << col));
        uint8_t br_closed = !(br_bus & (1 << col));

        uint8_t midi_note = 28 + key_idx; // Nota inicial E0 (28 em MIDI)

        switch (keys[key_idx].state) {
            case 0: // Repouso
                if (mk_closed) {
                    keys[key_idx].t1_tick = global_ticks;
                    keys[key_idx].state = 1; // Pressionando
                }
                break;

            case 1: // MK Fechado, aguardando BR
                if (br_closed) {
                    uint16_t delta_t = global_ticks - keys[key_idx].t1_tick;
                    if (delta_t > 127) delta_t = 127;

                    uint8_t vel = pgm_read_byte(&(velocity_lut[delta_t]));
                    midi_send(0x90, midi_note, vel); // Note On
                    keys[key_idx].state = 2;
                } else if (!mk_closed) {
                    // Desistência de toque ou ruído
                    keys[key_idx].state = 0;
                }
                break;

            case 2: // Tecla totalmente pressionada
                if (!mk_closed && !br_closed) {
                    midi_send(0x80, midi_note, 0); // Note Off
                    keys[key_idx].state = 0;
                }
                break;
        }
    }

    current_strobe = (current_strobe + 1) % NUM_STROBES;
}

int main(void) {
    uart_init();

    // Configura PORTL e PORTC como Saídas (Linhas de Varredura MK/BR)
    DDRL = 0xFF;  PORTL = 0xFF;
    DDRC = 0xFF;  PORTC = 0xFF;

    // Configura PORTA como Entrada com Pull-ups ativados (Sense/Colunas)
    DDRA = 0x00;  PORTA = 0xFF;

    // Configura TIMER1 para CTC a 5 kHz (Interrupção a cada 200 µs)
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11); // Prescaler = 8
    OCR1A = 1999;                          // (16 MHz / (8 * 5000 Hz)) - 1 = 399
    TIMSK1 = (1 << OCIE1A);

    sei(); // Habilita interrupções globais

    while (1) {
        // Loop principal 100% livre
    }
}
