#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

// Definições de hardware
#define LCD_I2C_ADDR    0x3F
#define LCD_COLUMNS     16
#define LCD_LINES       2
#define KEYPAD_ROWS     4
#define KEYPAD_COLS     4

#define pinLedRed   2
#define pinLedGreen 3
#define pinBuzzer   4

byte rowPins[KEYPAD_ROWS] = {5, 6, 7, 8};
byte colPins[KEYPAD_COLS] = {9, 10, 11, 12};
char keypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLUMNS, LCD_LINES);
Keypad keypad = Keypad(makeKeymap(keypadKeys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

// Constantes do jogo
const int BOMB_MAX_TIME_MINUTES = 1; // Tempo em minutos para o countdown
int countdownTime = BOMB_MAX_TIME_MINUTES * 60; // Tempo em segundos
bool isCountdownActive = false;
bool isStopRequest = false; // Para rastrear solicitação de parada

// Variáveis para detecção de pressionamento longo
unsigned long keyPressedTime = 0;
bool isKeyDPressed = false;



void setup() {
  Serial.begin(9600);

  checkLCD();
  checkLeds();
  checkBuzzer();

  Serial.println("CAM: Hardware check complete. Play!");
  screenFinishBoot();
}



void loop() {
  char key = keypad.getKey(); // Lê a tecla pressionada


  if (key) { // Verifica se alguma tecla foi pressionada
    startGame(key);
    disarmBombAction(key);
  } else { // Verifica se a tecla 'D' foi liberada
    if (isKeyDPressed && keypad.getState() == RELEASED) {
      isKeyDPressed = false;
    }
  }

  checkDisarmAction(isKeyDPressed);

  if (isCountdownActive) updateCountdownAndCheckBombStatus();
}





/*
#############################################################################
##                         CHECK HARDWARE                                  ##
#############################################################################
*/


void checkLCD() {
  lcd.init();
  lcd.backlight();
  Serial.println("CAM: Check LCD... Ready");
}


void checkBuzzer() {
  pinMode(pinBuzzer, OUTPUT);
  Serial.println("CAM: Check BUZZER... Ready");
}


void checkLeds() {
  pinMode(pinLedRed, OUTPUT);
  pinMode(pinLedGreen, OUTPUT);
  blinkLED(pinLedRed, 3, 200);
  blinkLED(pinLedGreen, 3, 200);
  Serial.println("CAM: Check LEDs... Ready");
  delay(500);
}




/*
#############################################################################
##                              UTILS                                      ##
#############################################################################
*/


void blinkLED(byte pinLed, byte blinkTimes, int delayTime) {
  digitalWrite(pinLed, LOW);
 
  for (byte i = 0; i < blinkTimes; i++) {
    digitalWrite(pinLed, HIGH);
    delay(delayTime);
    digitalWrite(pinLed, LOW);
    delay(delayTime);
  }
}


void showLCDMessage(String message, unsigned short posX, unsigned short posY) {
  lcd.setCursor(posX, posY);
  lcd.print(message);
}


void displayProgressBar(int progress) {
  lcd.setCursor(0, 1);
  lcd.print("[");
  for (int i = 0; i < LCD_COLUMNS - 2; i++) {
    if (i < progress) {
      lcd.print("#"); // Preenchimento da barra
    } else {
      lcd.print(" "); // Espaço vazio
    }
  }
  lcd.print("]");
}


void checkDisarmAction(boolean isKeyDPressed){
  if (isKeyDPressed) {
    unsigned long pressDuration = millis() - keyPressedTime;

    if (pressDuration >= 3000) { // Se pressionado por pelo menos 3 segundos
      isCountdownActive = false; // Para o timer
      isStopRequest = true;   // Flag para indicar parada
      lcd.clear();
      while(true){
        messageCTWin();
        delay(1000); // Aguarda um momento para mostrar a mensagem
      }
    } else {
      // Atualiza a barra de progresso
      int progress = (pressDuration * LCD_COLUMNS) / 3000; // Progresso em termos de caracteres (16 = largura do LCD)
      displayProgressBar(progress);
    }
  }
}





/*
#############################################################################
##                       TEXT MESSAGES                                     ##
#############################################################################
*/


void screenFinishBoot() {
  lcd.clear();

  showLCDMessage("CLUBE AIRSOFT", 1, 0);
  delay(500);
  showLCDMessage("MERTOLA", 4, 1);
  delay(3000);

  startGameMessage();
}


void startGameMessage(){
  lcd.clear();
  showLCDMessage("PRESSIONA 'A'", 1, 0);
  showLCDMessage("P/ ATIVAR BOMBA", 0, 1);
}


void startCountdownMessage() {
  lcd.clear();
  showLCDMessage("BOMBA", 5, 0);
  showLCDMessage("ACTIVADA", 4, 1);
  delay(1000);
}


void messageTWin(){
  showLCDMessage("BOMBA EXPLODIU", 1, 0);
  showLCDMessage("TERRORISTAS  WIN", 0, 1);
}


void messageCTWin(){
  showLCDMessage("BOMBA DESARMADA", 0, 0);
  showLCDMessage("CT WIN", 3, 1);
}





/*
#############################################################################
##          GAME: DEFUSE BOMB (COUNTER STRIKE MODE)                        ##
#############################################################################
*/


void startGame(char key){
  if (key == 'A' && !isCountdownActive) { // Verifica se é a tecla 'A' e inicia o countdown
    isCountdownActive = true;
    startCountdownMessage();
  }
}


void disarmBombAction(char key){
  if (key == 'D') { // Verifica se a tecla 'D' foi pressionada
    if (!isKeyDPressed) {
      isKeyDPressed = true;
      keyPressedTime = millis(); // Marca o tempo inicial
      displayProgressBar(0);     // Mostra a barra no início
    }
  }
}


void updateCountdownAndCheckBombStatus() {
  static unsigned long lastUpdateTime = 0;
  unsigned long currentTime = millis();

  // Atualiza o contador a cada segundo
  if (currentTime - lastUpdateTime >= 1000) {
    lastUpdateTime = currentTime;

    if (countdownTime > 0) {
      countdownTime--;
      int minutes = countdownTime / 60;
      int seconds = countdownTime % 60;

      lcd.clear();
      showLCDMessage("TEMPO RESTANTE", 1, 0);
      
      lcd.setCursor(5, 1);
      lcd.print(minutes);
      lcd.print("m ");
      lcd.print(seconds);
      lcd.print("s");
    } else {
      isCountdownActive = false;
      lcd.clear();
      
      while(true){
        messageTWin();
        // Opcional: Ativar o buzzer ou LED vermelho
        tone(pinBuzzer, 1000, 2000); // Beep de 2 segundos
      }
      
    }
  }
}

