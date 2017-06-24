#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#define D1 5
#define outDoor D1
#define n 10 //numero de pontos da média móvel

const char* ssid = "TIM WiFi Fon";
const char* password = "estoucomfome";

const char *mqtt_server = "m11.cloudmqtt.com"; //"192.168.1.10 //"m11.cloudmqtt.com";
const int mqtt_port = 14458; //1883; //14458
const char *mqtt_user = "jgojknep"; //"localBroker"; //jgojknep
const char *mqtt_pass = "muJlxlc58d0v"; //"1234"; //"muJlxlc58d0v"

const char *mqtt_client_name = "arduinoClient1"; // Client connections cant have the same connection name

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
int lastTemp, temp;
long now;
char msg[50];

const char *topics[] = {"keepAlive", "outTopic", "door", "temp", "mode"};
int numTopics;
String message;

int numbers[n];
long moving_avg();

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(A0, INPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.begin(115200);
  for (int i = 0; i < 200; i++){moving_avg(analogRead(A0));}
  temp = getTemp(moving_avg(analogRead(A0)));
  numTopics = (sizeof(topics) / sizeof(char*)) - 1;
  pinMode(outDoor, OUTPUT);
  digitalWrite(outDoor, LOW);
  delay(10);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  digitalWrite(BUILTIN_LED, LOW);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  message = String((char*)payload);
  Serial.println(message);
  if (String(topic) == "door" && message == "open") {
    Serial.println("Requisição para abrir portão");
    digitalWrite(outDoor, HIGH);
    delay(500);
    client.publish("door", "openSucess" );
    
    digitalWrite(outDoor, LOW);
  } else if (String(topic) == "keepAlive") {
    if (message == "NodeMcu/check" || message == "check") {
      client.publish("keepAlive", "NodeMcu/Alive");
      Serial.println("Send Keep Alive");
    }
  }else if (String(topic) == "temp"){
    if (message =="getTemp"){
      publishTemp(lastTemp);
      Serial.println("Send Temp");
    }
  }
  digitalWrite(BUILTIN_LED, HIGH);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("keepAlive", "NodeMcu/Alive");
      publishTemp(temp);
      // ... and resubscribe
      for (int i = 0; i <= numTopics; i++) {
        client.subscribe(topics[i]);
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  now = millis();
  if (now - lastMsg > 60*1000) {
    lastMsg = now;
    temp = getTemp(moving_avg(analogRead(A0)));
    if (lastTemp != temp){
      lastTemp = temp;
      publishTemp(temp);
    }else{
      Serial.print("Temperatura: ");
      Serial.print(temp);
      Serial.println("°C");
    }
  }
}

void publishTemp(int value) {
  String temp = String(value);
  String msg = "tempInter/";
  msg += temp;
  char message[58];
  msg.toCharArray(message, 58);
  client.publish("temp", message);
}

float getTemp(int value){
  float temperature = (value * 285.0f) / 1023.0f;
  return temperature;
}

long moving_avg(int value){
  for(int i = n-1; i>0; i--){numbers[i] = numbers[i-1];}
  numbers[0] = value;
  long acc = 0;
  for (int i = 0; i < n; i++){acc += numbers[i];}
  return acc/n;
}

