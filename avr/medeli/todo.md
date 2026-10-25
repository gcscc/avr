## teclas

começa em 28

- [ ] n=32 sem 1o contato
- [ ] n=43 sem 1o contato
- [ ] n=44 sem 1o contato

## cc

- [ ] inverter polaridade do pitchbend
- [ ] calibrar analógicos
- [ ] testar pedais

só uma dúvida pontual, não precisa considerar essa alteração agora nem escrever código para nada. é tranquilo subsituir as portas A8 ... A11 por 38 ... 42? Vai gerar conflito com algo? É mais pela conveniência de ter todas as linhas do keybed próximas e liberar as analógicas para uso.
Exploring alternate contiguous pin blocks to avoid conflicts.

Quase: o pino 42 já está em uso. Ele é o PL7, a linha 15 (MK7) da varredura. Como você precisa de 4 linhas, imagino que queira os pinos 38 a 41, que são justamente os que sobram entre o bloco do PORTC (30–37) e o do PORTL (42–49). Assim as 20 linhas ficam contíguas, de 30 a 49.

Com 38 a 41, não há conflito elétrico nem de função:
Pino	Porta	Observação
38	PD7	livre
39	PG2	livre
40	PG1	livre
41	PG0	livre

    Funções alternativas: nenhuma delas está ativa no firmware (nem o barramento externo, nem timers nesses pinos).
    Pedal soft: ele está no PG5 (pino 4), na mesma porta de 39, 40 e 41. Isso não conflita, mas a varredura não pode sobrescrever o PORTG inteiro sem preservar o pull-up do PG5.
    Analógicos: PK0 a PK3 (A8 a A11) ficam livres. O firmware atual usa PF0 a PF3 e não toca em PK, a não ser pela varredura que você estaria movendo.

O ponto que exige mudança de código: os 4 pinos ficam em duas portas (PD7 e PG0 a PG2), em vez de um bloco só como o PORTK. Hoje a varredura escreve PORTK de uma vez, então essa parte teria que ser reescrita, com os bits tratados individualmente. Mantendo o mesmo custo por linha e preservando o PG5, é viável, só deixa de ser uma troca de fios simples. Quando quiser, eu faço a alteração. Lembre também que, em cada grupo, a linha de número par tem que continuar sendo o 1º contato, e a ímpar o 2º, senão a velocity inverte.

Se a conveniência for mais de fiação do que de proximidade física, uma alternativa sem mexer na lógica é PB4 a PB7 (pinos 10 a 13), que são 4 bits contíguos da mesma porta. A troca no código seria mínima, só trocar o PORTK por PORTB com máscara. O inconveniente é que ficam longe do bloco de 30 a 49, e o pino 13 acende o LED da placa a cada varredura.
Faz sentido, esse é um problema real de perfboard. Os pinos 10 a 13 ficam num conector separado do bloco 22–53, com um deslocamento de meio passo, então não dá para soldar tudo num mesmo conector. Mas os pinos 38 a 41 não têm esse problema: eles fazem parte do mesmo bloco de 2 colunas com passo de 0,1" que os seus pinos 22–29, 30–37 e 42–49. Se o seu perfboard encaixa nos pinos que você já usa, encaixa nesses também. A matriz inteira (T e linhas) fica num único conector contínuo, de 22 a 49.

O que muda em relação ao que eu tinha dito

Eu tinha descartado 38–41 pela complexidade do código, mas olhando de novo o custo é pequeno e a performance não piora:

Velocidade: PORTD e PORTG estão no espaço baixo de I/O do ATmega2560, e a escrita leva 1 ciclo (out). O PORTK, que você usa hoje, leva 2 (sts). Então a varredura fica igual ou um pouco mais rápida.
Código: os 4 pinos ficam em duas portas (PD7 e PG0–PG2). A varredura trataria essas 4 linhas como dois trechos curtos, sem laço, em vez de um bloco de PORTK. Isso mexe numa parte já validada, então eu testaria de novo a varredura cromática, principalmente as 12 últimas teclas.
PG5 (pedal soft): ele está no mesmo PORTG. O código teria que manter o bit 5 em 1 em toda escrita, senão o pull-up do pedal cai.
Ordem das linhas: nada muda para a velocity. Se você mantiver fio a fio (o que estava em A8 vai para 38, A9 para 39, A10 para 40 e A11 para 41), eu mapeio as linhas 16 a 19 nessa ordem e as linhas pares seguem sendo o 1º contato.