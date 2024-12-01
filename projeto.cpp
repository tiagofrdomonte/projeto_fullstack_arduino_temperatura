#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <max6675.h>
#include <SPI.h>
#include <Ethernet.h>

// Pinos de conexão
#define pinoSO 8
#define pinoCS 9
#define pinoCLK 10
#define saidaRele 11
#define botaoAjuste 2
#define botaoSetaBaixo 3
#define botaoSetaCima 4

// Configurações do sistema
float setPoint = 35.0;
float histerese = 2.0;

bool telaInit = false;
bool ajustaSet = false;
bool ajustaHisterese = false;
bool controlando = false;

int telas = 0;

// Inicialização de sensores, display e rede
MAX6675 meuTermopar(pinoCLK, pinoCS, pinoSO);
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

byte mac[] = { "digite seu endereço MAC" };
IPAddress ip(192, 168, 1, 100);
EthernetServer server(80);

void setup() {
  // Inicialização de periféricos
  lcd.begin(16, 2);
  lcd.backlight();

  pinMode(saidaRele, OUTPUT);
  digitalWrite(saidaRele, HIGH);

  pinMode(botaoAjuste, INPUT);
  pinMode(botaoSetaBaixo, INPUT);
  pinMode(botaoSetaCima, INPUT);

  Serial.begin(9600);
  Serial.println("Iniciando controlador de temperatura...");

  // Inicializa Ethernet
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Servidor iniciado. Acesse: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  // Gerencia telas de controle
  if (telas == 0) {
    telaInicial();
  } else if (telas == 1) {
    telaControlandoTemperatura();
  } else if (telas == 2) {
    telaAjustaSet();
  } else if (telas == 3) {
    telaAjustaHisterese();
  }

  // Gerencia botões
  if (digitalRead(botaoAjuste) == HIGH) {
    while (digitalRead(botaoAjuste) == HIGH) {}
    telas = (telas + 1) % 4; // Alterna entre 4 telas
    resetScreen();
    delay(100);
  }

  // Gerencia conexões Ethernet
  EthernetClient client = server.available();
  if (client) {
    handleClient(client);
  }
}

void handleClient(EthernetClient client) {
  String currentLine = "";
  bool isGetDataRequest = false;

  // Lê a requisição HTTP do cliente
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      currentLine += c;

      // Verifica se o cliente está solicitando a rota "/data"
      if (currentLine.endsWith("GET /data HTTP/1.1\r\n")) {
        isGetDataRequest = true;
      }

      // Identifica fim do cabeçalho HTTP
      if (c == '\n' && currentLine == "\r\n") {
        if (isGetDataRequest) {
          // Resposta JSON para a requisição "/data"
          client.println("HTTP/1.1 200 OK");
client.println("Content-Type: application/json; charset=UTF-8");
client.println("Access-Control-Allow-Origin: *"); // Permite acesso de qualquer origem
client.println("Connection: close");
client.println();

          client.print("{\"tempAtual\":");
          client.print(meuTermopar.readCelsius());
          client.print(",\"setPoint\":");
          client.print(setPoint);
          client.print(",\"histerese\":");
          client.print(histerese);
          client.print(",\"releStatus\":\"");
          client.print(digitalRead(saidaRele) == LOW ? "ON" : "OFF");
          client.println("\"}");
        } else {
          // Resposta padrão para rota principal
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
client.println(R"rawliteral(
  <!DOCTYPE HTML>
  <html>
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
    <title>Projeto Fullstack</title>
    <style>
      body { font-family: Arial, sans-serif; background-color: #ececff;}
      h1 { text-align: center; color: #333; color: #0f07fe;}
      p { text-align: center; font-size: 20px; }
      .data { color: #ff6347; font-weight: bold; }
      #releStatus { font-size: 22px; }
      .card {
        width: 300px;
        height: 70px;
        border: 1px solid #ffffff;
        border-radius: 8px;
        background-color: #ffffff;
        box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
        overflow: hidden;
      }
      .container {
        display: flex;
        flex-wrap: wrap; /* Permite quebrar linha caso necessário */
        gap: 16px;       /* Espaçamento entre os cards */
        justify-content: center; /* Centraliza os cards dentro do contêiner */
      }
    </style>
  </head>
  <body>
    <h1>Controlador de Temperatura</h1>
    <div class="container">
      <div class="card">
        <p>Temperatura Atual: <span id="tempAtual" class="data">loading...</span> &deg;C</p>
      </div> 
      <div class="card">
        <p>Set Point: <span id="setPoint" class="data">loading...</span> &deg;C</p>
      </div>
      <div class="card">
        <p>Histerese: <span id="histerese" class="data">loading...</span> &deg;C</p>
      </div>
      <div class="card">
        <p>Sa&iacute;da do Rel&eacute;: <span id="releStatus">loading...</span></p>
      </div>
    </div>
    <script>
      function updateData() {
        fetch('/data')
          .then(response => response.json())
          .then(data => {
            document.getElementById('tempAtual').textContent = data.tempAtual;
            document.getElementById('setPoint').textContent = data.setPoint;
            document.getElementById('histerese').textContent = data.histerese;
            document.getElementById('releStatus').textContent = data.releStatus;
          });
      }
      setInterval(updateData, 1000);
    </script>
  </body>
</html>)rawliteral");


        }
        break;
      }

      if (c == '\n') {
        currentLine = "";
      }
    }
  }

  delay(1);
  client.stop();
}

void telaInicial() {
  if (!telaInit) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Aperte AJ para");
    lcd.setCursor(0, 1);
    lcd.print("comecar");
    Serial.println("Tela Inicial exibida.");
    telaInit = true;
  }
}

void telaControlandoTemperatura() {
  float temperaturaAtual = meuTermopar.readCelsius();

  // Controle do relé
  if (temperaturaAtual < setPoint - histerese) {
    digitalWrite(saidaRele, LOW); // Liga o relé
  } else if (temperaturaAtual > setPoint) {
    digitalWrite(saidaRele, HIGH); // Desliga o relé
  }

  // Atualização do display
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperaturaAtual, 1);
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("Set: ");
  lcd.print(setPoint, 1);
  lcd.print(" H: ");
  lcd.print(histerese, 1);
  
  lcd.setCursor(0, 3);
  lcd.print("Rele: ");
  lcd.print(digitalRead(saidaRele) == LOW ? "ON " : "OFF");

  delay(1000); // Aguarda 1 segundo antes de atualizar novamente
}

void telaAjustaSet() {
  if (!ajustaSet) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ajuste Set Point");
    lcd.setCursor(0, 1);
    lcd.print("Set: ");
    lcd.print(setPoint, 1);
    ajustaSet = true;
  }

  if (digitalRead(botaoSetaCima) == HIGH) {
    setPoint += 0.5; // Incrementa o setPoint
    while (digitalRead(botaoSetaCima) == HIGH) {} // Aguarda soltar o botão
    lcd.setCursor(5, 1);
    lcd.print("     ");
    lcd.setCursor(5, 1);
    lcd.print(setPoint, 1);
  }

  if (digitalRead(botaoSetaBaixo) == HIGH) {
    setPoint -= 0.5; // Decrementa o setPoint
    while (digitalRead(botaoSetaBaixo) == HIGH) {} // Aguarda soltar o botão
    lcd.setCursor(5, 1);
    lcd.print("     ");
    lcd.setCursor(5, 1);
    lcd.print(setPoint, 1);
  }
}

void telaAjustaHisterese() {
  if (!ajustaHisterese) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ajuste Histerese");
    lcd.setCursor(0, 1);
    lcd.print("H: ");
    lcd.print(histerese, 1);
    ajustaHisterese = true;
  }

  if (digitalRead(botaoSetaCima) == HIGH) {
    histerese += 0.1;
    while (digitalRead(botaoSetaCima) == HIGH) {}
    lcd.setCursor(3, 1);
    lcd.print("     ");
    lcd.setCursor(3, 1);
    lcd.print(histerese, 1);
  }

  if (digitalRead(botaoSetaBaixo) == HIGH) {
    histerese -= 0.1;
    while (digitalRead(botaoSetaBaixo) == HIGH) {}
    lcd.setCursor(3, 1);
    lcd.print("     ");
    lcd.setCursor(3, 1);
    lcd.print(histerese, 1);
  }
}

void resetScreen() {
  telaInit = false;
  ajustaSet = false;
  ajustaHisterese = false;
  controlando = false;
}
