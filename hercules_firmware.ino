#include <Arduino.h>
#include <AccelStepper.h>
#include "BluetoothSerial.h"

// =====================================================
// Bluetooth Serial
// =====================================================

BluetoothSerial SerialBT;

const char* BLUETOOTH_NAME = "Hercules_I";

// =====================================================
// Configuracoes dos motores 28BYJ-48 com ULN2003
// =====================================================

const long STEPS_PER_REV = 2048;

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

const float MOTOR1_MAX_SPEED_STEPS_S = 350.0;
const float MOTOR1_ACCEL_STEPS_S2 = 200.0;
const float MOTOR2_MAX_SPEED_STEPS_S = 500.0;
const float MOTOR2_ACCEL_STEPS_S2 = 250.0;

const long FIRING_STEPS = 2 * STEPS_PER_REV;
const unsigned long FIRING_SETTLE_MS = 1000;

// Ordem recomendada para 28BYJ-48 + ULN2003:
// IN1, IN3, IN2, IN4
AccelStepper motor1(AccelStepper::FULL4WIRE, M1_IN1, M1_IN3, M1_IN2, M1_IN4);
AccelStepper motor2(AccelStepper::FULL4WIRE, M2_IN1, M2_IN3, M2_IN2, M2_IN4);

// =====================================================
// Maquina de estados
// =====================================================

enum EstadoCatapulta {
  IDLE,
  ARMING,
  ARMED,
  FIRING_FORWARD,
  FIRING_SETTLE,
  FIRING_RETURN,
  DONE,
  ABORTED,
  ERROR
};

struct SistemaCatapulta {
  EstadoCatapulta estado = IDLE;
  bool armado = false;
  bool abortado = false;
  bool erro = false;
  float voltasSolicitadas = 0.0;
  long alvoMotor1 = 0;
  long alvoMotor2 = 0;
  unsigned long instanteEspera = 0;
  String ultimaFalha = "";
};

SistemaCatapulta sistema;

struct EntradaComandos {
  String buffer = "";
  unsigned long ultimoByteMs = 0;
};

EntradaComandos entradaSerial;
EntradaComandos entradaBluetooth;

// =====================================================
// Funcoes auxiliares
// =====================================================

void enviarMensagem(const String& msg) {
  Serial.println(msg);
  SerialBT.println(msg);
}

const char* nomeEstado(EstadoCatapulta estado) {
  switch (estado) {
    case IDLE:
      return "IDLE";
    case ARMING:
      return "ARMANDO";
    case ARMED:
      return "ARMADA";
    case FIRING_FORWARD:
      return "DISPARANDO";
    case FIRING_SETTLE:
      return "AGUARDANDO_RETORNO";
    case FIRING_RETURN:
      return "RETORNANDO";
    case DONE:
      return "FIM";
    case ABORTED:
      return "ABORTADO";
    case ERROR:
      return "ERRO";
  }

  return "DESCONHECIDO";
}

bool motorExecutando() {
  return sistema.estado == ARMING ||
         sistema.estado == FIRING_FORWARD ||
         sistema.estado == FIRING_SETTLE ||
         sistema.estado == FIRING_RETURN;
}

void liberarBobinas() {
  motor1.disableOutputs();
  motor2.disableOutputs();
}

void prepararMotor1() {
  motor1.enableOutputs();
}

void prepararMotor2() {
  motor2.enableOutputs();
}

void definirErro(const String& falha) {
  sistema.estado = ERROR;
  sistema.armado = false;
  sistema.erro = true;
  sistema.abortado = false;
  sistema.ultimaFalha = falha;
  liberarBobinas();
  enviarMensagem("ERRO: " + falha);
}

void abortarOperacao() {
  motor1.moveTo(motor1.currentPosition());
  motor2.moveTo(motor2.currentPosition());

  sistema.estado = ABORTED;
  sistema.armado = false;
  sistema.abortado = true;
  sistema.erro = false;
  sistema.ultimaFalha = "";

  liberarBobinas();
  enviarMensagem("ABORTADO");
}

void resetarSistema() {
  motor1.setCurrentPosition(0);
  motor2.setCurrentPosition(0);

  sistema.estado = IDLE;
  sistema.armado = false;
  sistema.abortado = false;
  sistema.erro = false;
  sistema.voltasSolicitadas = 0.0;
  sistema.alvoMotor1 = 0;
  sistema.alvoMotor2 = 0;
  sistema.instanteEspera = 0;
  sistema.ultimaFalha = "";

  liberarBobinas();
  enviarMensagem("RESET_OK");
}

void iniciarArmar(float voltas) {
  if (motorExecutando()) {
    enviarMensagem("ERRO: Sistema em movimento.");
    return;
  }

  if (sistema.estado == ABORTED || sistema.estado == ERROR) {
    enviarMensagem("ERRO: Envie RESET antes de armar novamente.");
    return;
  }

  if (voltas <= 0.0) {
    definirErro("Numero de voltas invalido.");
    return;
  }

  sistema.estado = ARMING;
  sistema.armado = false;
  sistema.abortado = false;
  sistema.erro = false;
  sistema.voltasSolicitadas = voltas;
  sistema.alvoMotor1 = motor1.currentPosition() + lround(-voltas * STEPS_PER_REV);

  prepararMotor1();
  motor1.moveTo(sistema.alvoMotor1);

  enviarMensagem("ARMANDO");
}

void iniciarDisparo() {
  if (motorExecutando()) {
    enviarMensagem("ERRO: Sistema em movimento.");
    return;
  }

  if (!sistema.armado || sistema.estado != ARMED) {
    enviarMensagem("ERRO: Sistema ainda nao esta armado.");
    return;
  }

  sistema.estado = FIRING_FORWARD;
  sistema.armado = false;
  sistema.alvoMotor2 = motor2.currentPosition() + FIRING_STEPS;

  prepararMotor2();
  motor2.moveTo(sistema.alvoMotor2);

  enviarMensagem("DISPARANDO");
}

void enviarStatus() {
  String status = "STATUS: ";
  status += nomeEstado(sistema.estado);

  if (sistema.estado == ERROR && sistema.ultimaFalha.length() > 0) {
    status += " - ";
    status += sistema.ultimaFalha;
  }

  enviarMensagem(status);
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

  if (comando == "ABORT") {
    abortarOperacao();
    return;
  }

  if (comando == "RESET") {
    resetarSistema();
    return;
  }

  if (comando == "STATUS") {
    enviarStatus();
    return;
  }

  if (comando.startsWith("ARMAR:")) {
    String valor = comando.substring(6);
    iniciarArmar(valor.toFloat());
    return;
  }

  if (comando == "DISPARAR") {
    iniciarDisparo();
    return;
  }

  enviarMensagem("ERRO: Comando desconhecido.");
}

void processarLinhasCompletas(EntradaComandos& entrada) {
  String& buffer = entrada.buffer;
  int fimLinha = buffer.indexOf('\n');

  while (fimLinha >= 0) {
    String comando = buffer.substring(0, fimLinha);
    buffer.remove(0, fimLinha + 1);
    tratarComando(comando);
    fimLinha = buffer.indexOf('\n');
  }
}

void processarComandoPendente(EntradaComandos& entrada) {
  if (entrada.buffer.length() == 0) {
    return;
  }

  if (millis() - entrada.ultimoByteMs < 30) {
    return;
  }

  String comando = entrada.buffer;
  entrada.buffer = "";
  tratarComando(comando);
}

void lerComandos(Stream& origem, EntradaComandos& entrada) {
  while (origem.available()) {
    char c = origem.read();
    entrada.buffer += c;
    entrada.ultimoByteMs = millis();
  }

  processarLinhasCompletas(entrada);
  processarComandoPendente(entrada);
}

// =====================================================
// Atualizacao da FSM
// =====================================================

void atualizarEstado() {
  motor1.run();
  motor2.run();

  switch (sistema.estado) {
    case IDLE:
    case ARMED:
    case DONE:
    case ABORTED:
    case ERROR:
      break;

    case ARMING:
      if (motor1.distanceToGo() == 0) {
        motor1.disableOutputs();
        sistema.estado = ARMED;
        sistema.armado = true;
        enviarMensagem("ARMADA S");
      }
      break;

    case FIRING_FORWARD:
      if (motor2.distanceToGo() == 0) {
        sistema.estado = FIRING_SETTLE;
        sistema.instanteEspera = millis();
      }
      break;

    case FIRING_SETTLE:
      if (millis() - sistema.instanteEspera >= FIRING_SETTLE_MS) {
        sistema.estado = FIRING_RETURN;
        sistema.alvoMotor2 = motor2.currentPosition() - FIRING_STEPS;
        motor2.moveTo(sistema.alvoMotor2);
      }
      break;

    case FIRING_RETURN:
      if (motor2.distanceToGo() == 0) {
        motor2.setCurrentPosition(0);
        motor2.disableOutputs();
        sistema.estado = DONE;
        enviarMensagem("FIM");
      }
      break;
  }
}

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);

  SerialBT.begin(BLUETOOTH_NAME);
  SerialBT.setTimeout(20);

  motor1.setMaxSpeed(MOTOR1_MAX_SPEED_STEPS_S);
  motor1.setAcceleration(MOTOR1_ACCEL_STEPS_S2);
  motor2.setMaxSpeed(MOTOR2_MAX_SPEED_STEPS_S);
  motor2.setAcceleration(MOTOR2_ACCEL_STEPS_S2);

  liberarBobinas();

  enviarMensagem("ESP32 iniciado.");
  enviarMensagem("Bluetooth: Hercules_I");
  enviarMensagem("Comandos disponiveis:");
  enviarMensagem("ARMAR:numero_de_voltas");
  enviarMensagem("DISPARAR");
  enviarMensagem("ABORT");
  enviarMensagem("RESET");
  enviarMensagem("STATUS");
}

// =====================================================
// Loop principal
// =====================================================

void loop() {
  lerComandos(Serial, entradaSerial);
  lerComandos(SerialBT, entradaBluetooth);
  atualizarEstado();
}