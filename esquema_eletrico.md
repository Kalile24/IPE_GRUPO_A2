# Esquema Elétrico — Hércules I

Ligações do projeto: ESP32 DevKit V1 + 2 motores 28BYJ-48 + 2 drivers ULN2003

---

## 1. Componentes

| Componente              | Qtd | Observação                        |
|-------------------------|-----|-----------------------------------|
| ESP32 DevKit V1         | 1   | Controlador central / Bluetooth   |
| Motor de passo 28BYJ-48 | 2   | 5V, unipolar, 64 passos/volta     |
| Módulo driver ULN2003   | 2   | Acionamento dos motores           |
| Fonte externa 5V        | 1   | Alimentação dos motores           |
| Jumpers                 | —   | Conexões entre componentes        |
| Protoboard              | 1   | Montagem do circuito              |

---

## 2. Motor 1 — Armação

O Motor 1 é responsável pela etapa de armação (tensionamento da catapulta).

**Ligações ESP32 → ULN2003 Motor 1:**

| ESP32 | ULN2003 Motor 1 |
|-------|-----------------|
| D26   | IN1             |
| D27   | IN2             |
| D14   | IN3             |
| D12   | IN4             |

> Motor 28BYJ-48 conectado ao conector branco do módulo ULN2003 Motor 1.

---

## 3. Motor 2 — Disparo / Retorno

O Motor 2 é responsável pela etapa de disparo e retorno à posição inicial.

**Ligações ESP32 → ULN2003 Motor 2:**

| ESP32 | ULN2003 Motor 2 |
|-------|-----------------|
| D18   | IN1             |
| D19   | IN2             |
| D21   | IN3             |
| D22   | IN4             |

> Motor 28BYJ-48 conectado ao conector branco do módulo ULN2003 Motor 2.

---

## 4. Indicação de Estado

O firmware atual não usa LED externo dedicado no ESP32.

A indicação física durante movimento é feita pelos LEDs dos módulos ULN2003, que acendem conforme as bobinas dos motores são energizadas. O estado lógico do sistema é informado pelo aplicativo e pelo monitor serial com mensagens como `ARMANDO`, `ARMADA`, `DISPARANDO`, `FIM`, `ABORTADO` e `ERRO`.

---

## 5. Alimentação dos Motores

Os motores **não** devem ser alimentados diretamente pelo ESP32, pois a corrente necessária excede a capacidade dos pinos GPIO.

**Ligações da fonte externa 5V:**

| Fonte externa 5V | Destino              |
|------------------|----------------------|
| Positivo (+)     | VCC do ULN2003 Motor 1 |
| Positivo (+)     | VCC do ULN2003 Motor 2 |
| Negativo (−)     | GND do ULN2003 Motor 1 |
| Negativo (−)     | GND do ULN2003 Motor 2 |

> O driver ULN2003 pode aceitar tensões maiores, mas o motor usado é 28BYJ-48 5VDC. A tensão nominal do motor deve prevalecer. Testes com 4 pilhas de 1,5V geram aproximadamente 6V e devem ser feitos com atenção ao aquecimento.

---

## 6. GND Comum

O GND da fonte externa deve estar ligado ao GND do ESP32 para que os sinais de controle tenham a mesma referência elétrica.

**Todos os pontos a seguir devem estar interligados:**

- GND do ESP32
- GND da fonte externa 5V
- GND do ULN2003 Motor 1
- GND do ULN2003 Motor 2

---

## 7. Alimentação do ESP32

O ESP32 é alimentado via porta USB. Opções:

- Cabo USB conectado ao computador (durante desenvolvimento)
- Carregador USB 5V
- Power bank USB (uso em campo, sem computador)

---

## 8. Resumo das Ligações

### Motores

| Pino ESP32 | Motor | Pino ULN2003 |
|------------|-------|--------------|
| D26        | M1    | IN1          |
| D27        | M1    | IN2          |
| D14        | M1    | IN3          |
| D12        | M1    | IN4          |
| D18        | M2    | IN1          |
| D19        | M2    | IN2          |
| D21        | M2    | IN3          |
| D22        | M2    | IN4          |

### Alimentação

| Origem           | Destino                          |
|------------------|----------------------------------|
| Fonte 5V (+)     | VCC dos dois módulos ULN2003     |
| Fonte 5V (−)     | GND dos dois módulos ULN2003     |
| GND ESP32        | GND dos dois módulos ULN2003     |
| USB              | Alimentação do ESP32             |
