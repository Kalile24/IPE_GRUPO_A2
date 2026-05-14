# Hércules I — Catapulta Onager com Controle Bluetooth

Projeto acadêmico da disciplina **Introdução a Projetos de Engenharia I (IPE I)**
Instituto Militar de Engenharia (IME) — Turma 2026.1

---

## Descrição

O **Hércules I** é uma catapulta tipo *onager* construída com palitos de picolé, controlada remotamente via Bluetooth Clássico por um aplicativo Android. O objetivo é lançar uma esfera metálica para acertar alvos entre **0,5 m e 4,0 m** de distância.

---

## Equipe A2

| Membro     | Função                          |
|------------|---------------------------------|
| Kalile     | Gerente de Projeto / Firmware   |
| Lima       | Projeto Mecânico                |
| Adeodato   | Eletrônica                      |
| Rodrigues  | Construção e Testes             |

---

## Arquitetura do Sistema

```
Aplicativo Android (MIT App Inventor)
        |
        | Bluetooth Clássico (HC-05 / ESP32 BT)
        |
   ESP32 DevKit V1
        |
   +---------+---------+
   |                   |
Motor 1 (28BYJ-48)  Motor 2 (28BYJ-48)
[Armação]           [Disparo / Retorno]
   |                   |
Driver ULN2003      Driver ULN2003
```

---

## Hardware

| Componente           | Quantidade | Função                        |
|----------------------|------------|-------------------------------|
| ESP32 DevKit V1      | 1          | Controlador central / BT      |
| Motor de passo 28BYJ-48 | 2       | Armação e disparo             |
| Driver ULN2003       | 2          | Acionamento dos motores       |
| Fonte externa 5V     | 1          | Alimentação dos motores       |

---

## Pinagem ESP32

### Motor 1 — Armação

| ESP32 | ULN2003 Motor 1 |
|-------|-----------------|
| D26   | IN1             |
| D27   | IN2             |
| D14   | IN3             |
| D12   | IN4             |

### Motor 2 — Disparo / Retorno

| ESP32 | ULN2003 Motor 2 |
|-------|-----------------|
| D18   | IN1             |
| D19   | IN2             |
| D21   | IN3             |
| D22   | IN4             |

---

## Alimentação

- **Motores:** fonte externa 5V → VCC dos módulos ULN2003
- **ESP32:** USB (computador, carregador ou power bank)
- **GND comum:** GND da fonte externa conectado ao GND do ESP32 e ao GND dos dois módulos ULN2003

> Os motores **não** devem ser alimentados diretamente pelo ESP32.

---

## Software

### Firmware (ESP32)

Arquivo: [`hercules_firmware.ino`](hercules_firmware.ino)

Desenvolvido em C++ com a Arduino IDE. O ESP32 recebe comandos via Bluetooth Serial e aciona os motores de passo por meio dos drivers ULN2003.

Bibliotecas usadas:
- `BluetoothSerial`
- `AccelStepper`

Comandos aceitos:
- `ARMAR:numero_de_voltas` — arma a catapulta com o Motor 1.
- `DISPARAR` — aciona o Motor 2 por 2 voltas e retorna 2 voltas.
- `ABORT` — interrompe a operação e libera as bobinas.
- `RESET` — limpa estado de erro/aborto.
- `STATUS` — informa o estado atual.

Mensagens principais enviadas ao app:
- `ARMANDO`
- `ARMADA`
- `DISPARANDO`
- `FIM`
- `ABORTADO`
- `ERRO`

O Motor 1 não retorna após a armação, pois a retenção é feita pelo conjunto mecânico com engrenagens. O Motor 2 realiza o ciclo de disparo e retorno.

### Aplicativo Android

Arquivo: [`Hercules_I_app.aia`](Hercules_I_app.aia)

Desenvolvido no **MIT App Inventor**. O arquivo `.aia` pode ser importado diretamente na plataforma para edição ou compilado para `.apk`.

Funcionalidades do app:
- Conectar ao ESP32 via Bluetooth
- Armar a catapulta (Motor 1)
- Disparar (Motor 2)
- Habilitar o disparo quando o ESP32 envia `ARMADA`
- Retornar à posição inicial após o disparo do Motor 2

O status operacional é indicado pelas mensagens no app e pelos LEDs dos módulos ULN2003 durante a energização das bobinas. Não há LED externo dedicado no firmware atual.

---

## Esquema Elétrico

Consulte [`esquema_eletrico.md`](esquema_eletrico.md) para o detalhamento completo das ligações.

---

## Estrutura do Repositório

```
hercules-i/
├── README.md                  # Este arquivo
├── hercules_firmware.ino      # Firmware do ESP32
├── Hercules_I_app.aia         # Projeto do app Android (MIT App Inventor)
└── esquema_eletrico.md        # Esquema elétrico e ligações
```

---

## Licença

Uso acadêmico — Instituto Militar de Engenharia, 2026.
