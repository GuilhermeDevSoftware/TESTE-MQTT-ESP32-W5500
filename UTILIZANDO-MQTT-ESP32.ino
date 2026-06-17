#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// ==================================================
// PINOS DO W5500
// ==================================================

#define PINO_SCK   18
#define PINO_MISO  19
#define PINO_MOSI  23
#define PINO_CS     5
#define PINO_RST   26

// ==================================================
// CONFIGURAÇÃO DE REDE
// ==================================================

// Endereço MAC local do W5500.
// Deve ser único dentro da rede.
byte mac[] = {
  0x02, 0x47, 0x41, 0x54, 0x45, 0x01
};

// ALTERE PARA O IP DO SEU COMPUTADOR
IPAddress ipBroker(192, 168, 100, 17);

const uint16_t portaBroker = 1883;

// ==================================================
// CLIENTES ETHERNET E MQTT
// ==================================================

EthernetClient clienteEthernet;
PubSubClient clienteMQTT(clienteEthernet);

// ==================================================
// CONTROLE DE TEMPO
// ==================================================

unsigned long ultimaPublicacao = 0;
unsigned long ultimaTentativaMQTT = 0;
unsigned long contador = 0;

// ==================================================
// RESET DO W5500
// ==================================================

void reiniciarW5500() {
  pinMode(PINO_RST, OUTPUT);

  digitalWrite(PINO_RST, LOW);
  delay(200);

  digitalWrite(PINO_RST, HIGH);
  delay(500);
}

// ==================================================
// MOSTRAR ENDEREÇO IP
// ==================================================

void mostrarEnderecoIP() {
  Serial.print("IP do ESP32: ");
  Serial.println(Ethernet.localIP());

  Serial.print("Gateway: ");
  Serial.println(Ethernet.gatewayIP());

  Serial.print("Mascara: ");
  Serial.println(Ethernet.subnetMask());

  Serial.print("IP do broker MQTT: ");
  Serial.println(ipBroker);
}

// ==================================================
// CONECTAR AO MQTT
// ==================================================

bool conectarMQTT() {
  Serial.println();
  Serial.print("Conectando ao Mosquitto em ");
  Serial.print(ipBroker);
  Serial.print(":");
  Serial.println(portaBroker);

  bool conectado = clienteMQTT.connect(
    "gateway-ambiental-esp32"
  );

  if (conectado) {
    Serial.println("MQTT conectado!");

    clienteMQTT.publish(
      "gateway/ambiental/status",
      "ESP32 conectado via W5500",
      true
    );

    return true;
  }

  Serial.print("Falha MQTT. Codigo: ");
  Serial.println(clienteMQTT.state());

  return false;
}

// ==================================================
// SETUP
// ==================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("====================================");
  Serial.println("TESTE ESP32 + W5500 + MQTT");
  Serial.println("====================================");

  reiniciarW5500();

  SPI.begin(
    PINO_SCK,
    PINO_MISO,
    PINO_MOSI,
    PINO_CS
  );

  Ethernet.init(PINO_CS);

  Serial.println("Obtendo endereco IP por DHCP...");

  if (Ethernet.begin(mac) == 0) {
    Serial.println("ERRO: nao foi possivel obter IP por DHCP.");
    Serial.println("Verifique o cabo, o roteador e o W5500.");

    while (true) {
      delay(1000);
    }
  }

  delay(1000);

  mostrarEnderecoIP();

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("ERRO: W5500 nao encontrado.");

    while (true) {
      delay(1000);
    }
  }

  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("AVISO: cabo Ethernet desconectado.");
  }

  clienteMQTT.setServer(ipBroker, portaBroker);
  clienteMQTT.setKeepAlive(30);
  clienteMQTT.setBufferSize(256);

  conectarMQTT();
}

// ==================================================
// LOOP
// ==================================================

void loop() {
  Ethernet.maintain();

  // Tenta reconectar ao MQTT a cada 5 segundos
  if (!clienteMQTT.connected()) {
    if (millis() - ultimaTentativaMQTT >= 5000) {
      ultimaTentativaMQTT = millis();
      conectarMQTT();
    }
  }

  clienteMQTT.loop();

  // Publica uma mensagem a cada 5 segundos
  if (clienteMQTT.connected() &&
      millis() - ultimaPublicacao >= 5000) {

    ultimaPublicacao = millis();

    char mensagem[50];

    snprintf(
      mensagem,
      sizeof(mensagem),
      "Pacote %lu - W5500 funcionando",
      contador
    );

    bool enviado = clienteMQTT.publish(
      "gateway/ambiental/teste",
      mensagem
    );

    if (enviado) {
      Serial.print("Publicado: ");
      Serial.println(mensagem);
      contador++;
    } else {
      Serial.println("Falha ao publicar mensagem.");
    }
  }
}