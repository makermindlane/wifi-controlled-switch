#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <LiquidCrystal_I2C.h>


#define led LED_BUILTIN
#define relay_1 D5
#define relay_2 D6

//Different states of the connection
enum {INIT = 1, WAITING, CONNECTED, DISCONNECTED, UNABLE_TO_CONNECT, STOP};

const char* ssid = "espserver";
const char* password = "12345678";

int state;
bool prevRelay1State;
bool prevRelay2State;
bool currRelay1State;
bool currRelay2State;
bool prevBacklightState;
bool currBacklightState;

IPAddress ip;
// Start a TCP Server on port 80
WiFiServer server(80);
//WiFiClient instance to handle the connected client
WiFiClient client;
//lcd object to handle the lcd operations
LiquidCrystal_I2C lcd(0x3F, 16, 2);


// make some custom characters:
byte heart[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

/*byte smiley[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b10001,
  0b01110,
  0b00000
};*/

byte frownie[8] = {
  0b00000,
  0b00000,
  0b01010,
  0b00000,
  0b00000,
  0b00000,
  0b01110,
  0b10001
};

byte armsDown[8] = {
  0b00100,
  0b01010,
  0b00100,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b01010
};

byte armsUp[8] = {
  0b00100,
  0b01010,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b00100,
  0b01010
};


void showWelcomeScreen() {
  // create a new character
  lcd.createChar(0, heart);
  // create a new character
  //lcd.createChar(1, smiley);
  // create a new character
  lcd.createChar(2, frownie);
  // create a new character
  lcd.createChar(3, armsDown);
  // create a new character
  lcd.createChar(4, armsUp);

  // set the cursor to the top left
  lcd.setCursor(0, 0);

  // Print a message to the lcd.
  lcd.print("I ");
  lcd.write(byte(0)); // when calling lcd.write() '0' must be cast as a byte
  lcd.print(" ENGINEERING ");
  //lcd.write((byte)1);

  int timer = 0;

  while (timer < 3) {
    int delayTime = 800;
    // set the cursor to the bottom row, 5th position:
    lcd.setCursor(4, 1);
    // draw the little man, arms down:
    lcd.write(3);
    delay(delayTime);
    lcd.setCursor(4, 1);
    // draw him arms up:
    lcd.write(4);
    delay(delayTime);
    timer++;
  }

  delay(200);
}


void showInitProcessMsg() {
  lcd.clear();
  lcd.print("Initiating");
  lcd.setCursor(0, 1);
  lcd.print("Process...");
  delay(3000);
}


//Function to setup and configure wifi in AP mode
void setupWifi() {

  WiFi.softAP(ssid, password);        //Setup AP mode with ssid and password
  ip = WiFi.softAPIP();
  // Start the TCP server
  server.begin();
}


void showWifiInfo() {
  lcd.clear();
  lcd.print("IP: ");
  lcd.print(ip);
  lcd.setCursor(0, 1);
  lcd.print("Port: 80");
}


//function to set initial values of the relays and led(s)
void init() {

  //Setting all the devices into the off state (i.e., HIGH, because they are acive low)

  pinMode(led, OUTPUT);
  pinMode(relay_1, OUTPUT);
  pinMode(relay_2, OUTPUT);
  digitalWrite(relay_1, HIGH);
  digitalWrite(relay_2, HIGH);

  lcd.begin();
  lcd.backlight();
  setupWifi();

  currRelay1State = false;
  currRelay2State = false;
  currBacklightState = true;

  state = WAITING;

}


//Function to handle client connection
void connectToClient() {

  int timer = 0;

  client = server.available();

  while (!client && (timer < 60)) {
    client = server.available();
    digitalWrite(led, !digitalRead(led));       //making the built in led to blink during the WAITING state.
    timer++;
    showWifiInfo();
    lcd.setCursor(9, 1);
    lcd.print(timer);
    lcd.setCursor(12, 1);
    lcd.print("Sec");
    delay(1000);
  }
  //If 1 min has passed and no client has connected then, reaturn false.
  if (timer >= 60) {
    state = UNABLE_TO_CONNECT;
    return;
  }
  state = CONNECTED;

}


//Function to handle functionalities
void handleFunctionality(String cmd) {

  prevRelay1State = currRelay1State;
  prevRelay2State = currRelay2State;
  prevBacklightState = currBacklightState;

  lcd.clear();
  lcd.home();
  lcd.print("RELAY 1:");
  lcd.print(((prevRelay1State) ? "H" : "L"));
  lcd.setCursor(12, 0);
  lcd.print("BG:");
  lcd.print(((prevBacklightState) ? "H" : "L"));
  lcd.setCursor(0, 1);
  lcd.print("RELAY 2:");
  lcd.print(((prevRelay2State) ? "H" : "L"));


  if (cmd == "A") {
    //Turning first relay on
    lcd.setCursor(8, 0);
    lcd.print("H");
    currRelay1State = true;
    digitalWrite(relay_1, LOW);
  }
  else if (cmd == "a") {
    //Turning first relay Off
    lcd.setCursor(8, 0);
    lcd.print("L");
    currRelay1State = false;
    digitalWrite(relay_1, HIGH);
  }
  else if (cmd == "B") {
    //Turning second relay on
    lcd.setCursor(8, 1);
    lcd.print("H");
    currRelay2State = true;
    digitalWrite(relay_2, LOW);
  }
  else if (cmd == "b") {
    //Turning second relay off
    lcd.setCursor(8, 1);
    lcd.print("L");
    currRelay2State = false;
    digitalWrite(relay_2, HIGH);
  }
  else if (cmd == "C") {
    lcd.setCursor(15, 0);
    lcd.print("H");
    currBacklightState = true;
    lcd.backlight();
  }
  else if (cmd == "c") {
    lcd.setCursor(15, 0);
    lcd.print("L");
    currBacklightState = false;
    lcd.noBacklight();
  }
}


//Function to handle client data
void handleClientData () {

  //Getting data while client is connected.
  while (client.connected()) {
    //if client is sending some data
    if (client.available()) {
      //store that data
      String cmd = client.readStringUntil('\n');
      //Implement the functionality.
      handleFunctionality(cmd);
      client.flush();
      delay(200);
    }
  }
  state = DISCONNECTED;
}


void clientDisconnected() {

  lcd.clear();
  lcd.print("Client");
  lcd.setCursor(0, 1);
  lcd.print("Disconnected!!");
  client.stop();

  if (digitalRead(relay_1) == LOW) {
    digitalWrite(relay_1, HIGH);
  }
  if (digitalRead(relay_2) == LOW) {
    digitalWrite(relay_2, HIGH);
  }
  if (digitalRead(led) == LOW) {
    digitalWrite(led, HIGH);
  }

  delay(2000);

  state = STOP;

}


void setup() {

  init();
  showWelcomeScreen();
  showInitProcessMsg();
  showWifiInfo();

}


void loop() {

  switch (state) {
    case INIT:
      init();
      showWelcomeScreen();
      showInitProcessMsg();
      showWifiInfo();
      break;

    case WAITING:
      connectToClient();
      break;

    case CONNECTED:
      digitalWrite(led, LOW);
      lcd.clear();
      lcd.print("Connected!!!");
      handleClientData();
      break;

    case DISCONNECTED:
      clientDisconnected();
      break;

    case UNABLE_TO_CONNECT:
      //ESP.deepSleep(20e6);
      //Wait for 5 seconds before repeating the whole process
      lcd.clear();
      lcd.print("Unable To");
      lcd.setCursor(0, 1);
      lcd.print("Connect!!!");
      delay(2000);
      lcd.clear();
      lcd.print("Retrying!!!");
      delay(2000);
      state = WAITING;
      break;

    case STOP:
      state = INIT;
      break;

    default:
      lcd.print("UNKNOWN STATE!!!");
      delay(2000);
      break;
  }

}
