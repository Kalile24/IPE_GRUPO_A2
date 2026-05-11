#include <Arduino.h>
#include <Stepper.h>
#include "BluetoothSerial.h"

// =====================================================
// Bluetooth Serial
// =====================================================

BluetoothSerial SerialBT;

// Nome que aparecerá no Bluetooth do celular
const char* BLUETOOTH_NAME = "Hercules_I";

// =====================================================
// Configurações dos motores 28BYJ-48 com ULN2003
// =====================================================

// 28BYJ-48 normalmente usa 2048 passos por volta
const int STEPS_PER_REV = 2048;

// Motor 1 - Armar
const int M1_IN1 = 26;
const int M1_IN2 = 27;
const int M1_IN3 = 14;
const int M1_IN4 = 12;

// Motor 2 - Disparar / Retornar
const int M2_IN1 = 18;
const int M2_IN2 = 19;
const int M2_IN3 = 21;
const int M2_IN4 = 22;

// LED de status do ESP32
const int LED_STATUS = 2;

// Ordem recomendada para 28BYJ-48 + ULN2003:
// IN1, IN3, IN2, IN4
Stepper motor1(STEPS_PER_REV, M1_IN1, M1_IN3, M1_IN2, M1_IN4);
Stepper motor2(STEPS_PER_REV, M2_IN1, M2_IN3, M2_IN2, M2_IN4);

// =====================================================
// Estados do sistema
// =====================================================

bool sistemaArmado = false;
bool motorExecutando = false;

// Velocidade dos motores em RPM
const int MOTOR1_RPM = 10;
const int MOTOR2_RPM = 10;

// =====================================================
// Funções auxiliares
// =====================================================

void enviarMensagem(String msg) {
  Serial.println(msg);     // Envia para o Serial Monitor
  SerialBT.println(msg);   // Envia para o app via Bluetooth
}

void ligarStatus() {
  digitalWrite(LED_STATUS, HIGH);
}

void desligarStatus() {
  digitalWrite(LED_STATUS, LOW);
}

void liberarBobinasMotor1() {
  digitalWrite(M1_IN1, LOW);
  digitalWrite(M1_IN2, LOW);
  digitalWrite(M1_IN3, LOW);
  digitalWrite(M1_IN4, LOW);
}

void liberarBobinasMotor2() {
  digitalWrite(M2_IN1, LOW);
  digitalWrite(M2_IN2, LOW);
  digitalWrite(M2_IN3, LOW);
  digitalWrite(M2_IN4, LOW);
}

void liberarBobinas() {
  liberarBobinasMotor1();
  liberarBobinasMotor2();
}

// =====================================================
// Motor 1 - Armar
// =====================================================

void executarArmar(float voltas) {
  if (motorExecutando) {
    enviarMensagem("ERRO: Motor ja esta executando.");
    return;
  }

  if (voltas <= 0) {
    enviarMensagem("ERRO: Numero de voltas invalido.");
    return;
  }

  motorExecutando = true;
  sistemaArmado = false;

  long passos = round(voltas * STEPS_PER_REV);

  enviarMensagem("ARMANDO");
  ligarStatus();

  // Motor 1 gira o número de voltas definido no app
  motor1.step(passos);

  // Espera 1 segundo
  delay(1000);

  // Motor 1 retorna o mesmo número de voltas
  motor1.step(-passos);

  desligarStatus();
  liberarBobinasMotor1();

  sistemaArmado = true;
  motorExecutando = false;

  // Sinal enviado para o app habilitar o botão Lançar
  enviarMensagem("S");
}

// =====================================================
// Motor 2 - Disparar
// =====================================================

void executarDisparo() {
  if (motorExecutando) {
    enviarMensagem("ERRO: Motor ja esta executando.");
    return;
  }

  if (!sistemaArmado) {
    enviarMensagem("ERRO: Sistema ainda nao esta armado.");
    return;
  }

  motorExecutando = true;
  sistemaArmado = false;

  enviarMensagem("DISPARANDO");
  ligarStatus();

  // 180 graus = meia volta
  int passos180 = STEPS_PER_REV / 2;

  // Motor 2 gira 180 graus
  motor2.step(passos180);

  // Espera 3 segundos
  delay(3000);

  // Motor 2 retorna 180 graus
  motor2.step(-passos180);

  desligarStatus();
  liberarBobinasMotor2();

  motorExecutando = false;

  enviarMensagem("FIM");
}

// =====================================================
// Tratamento de comandos
// =====================================================

void tratarComando(String comando) {
  comando.trim();
  comando.toUpperCase();

  if (comando.length() == 0) {
    return;
  }

  Serial.print("Comando recebido: ");
  Serial.println(comando);

  // Comando vindo do app:
  // ARMAR:3
  // ARMAR:2.5
  if (comando.startsWith("ARMAR:")) {
    String valor = comando.substring(6);
    float voltas = valor.toFloat();

    executarArmar(voltas);
    return;
  }

  // Comando vindo do app:
  // DISPARAR
  if (comando == "DISPARAR") {
    executarDisparo();
    return;
  }

  // Comando de teste pelo Serial Monitor
  if (comando == "STATUS") {
    if (motorExecutando) {
      enviarMensagem("STATUS: EXECUTANDO");
    } else if (sistemaArmado) {
      enviarMensagem("STATUS: ARMADO");
    } else {
      enviarMensagem("STATUS: NAO_ARMADO");
    }
    return;
  }

  enviarMensagem("ERRO: Comando desconhecido.");
}

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(115200);

  // Inicia Bluetooth clássico do ESP32
  SerialBT.begin(BLUETOOTH_NAME);

  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_STATUS, LOW);

  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M1_IN3, OUTPUT);
  pinMode(M1_IN4, OUTPUT);

  pinMode(M2_IN1, OUTPUT);
  pinMode(M2_IN2, OUTPUT);
  pinMode(M2_IN3, OUTPUT);
  pinMode(M2_IN4, OUTPUT);

  motor1.setSpeed(MOTOR1_RPM);
  motor2.setSpeed(MOTOR2_RPM);

  liberarBobinas();

  enviarMensagem("ESP32 iniciado.");
  enviarMensagem("Bluetooth: Catapulta_ESP32");
  enviarMensagem("Comandos disponiveis:");
  enviarMensagem("ARMAR:numero_de_voltas");
  enviarMensagem("DISPARAR");
  enviarMensagem("STATUS");
}

// =====================================================
// Loop principal
// =====================================================

void loop() {
  // Teste pelo Serial Monitor da Arduino IDE
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    tratarComando(comando);
  }

  // Comandos recebidos pelo app via Bluetooth
  if (SerialBT.available()) {
    String comando = SerialBT.readStringUntil('\n');
    tratarComando(comando);
  }
}