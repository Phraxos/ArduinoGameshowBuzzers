#include <SoftwareSerial.h>

// Buzzers
const int buzzerOne = 12;
const int buzzerTwo = 13;

// Wireless operator remote
const int buttonOne = 8;
const int buttonTwo = 9;
const int buttonThree = 10;
const int buttonFour = 11;

// Sirens
int sirenOne = 6;
int sirenTwo = 7;

// Sound
#define AUDIO_RX 5
#define AUDIO_TX 4
SoftwareSerial mp3(AUDIO_RX, AUDIO_TX);
static int8_t Send_buf[8] = {0}; // Buffer for Send commands
static uint8_t ansbuf[10] = {0}; // Buffer for the answers
String mp3Answer;           // Answer from the MP3.

// Buzzer Settings
const int debounceSample = 20;
const int debounceSize = 15;

// Siren Settings
const unsigned long sirenMs = 6000; // how long to run siren
unsigned long sirenStart;
unsigned long sirenStop;

// State
int buzzerOneState;
int buzzerTwoState;
int buttonOneState;
int buttonTwoState;
int buttonThreeState;
int buttonFourState;

/************ MP3 **************************/
#define CMD_NEXT_SONG     0X01  // Play next song.
#define CMD_PREV_SONG     0X02  // Play previous song.
#define CMD_PLAY_W_INDEX  0X03
#define CMD_VOLUME_UP     0X04
#define CMD_VOLUME_DOWN   0X05
#define CMD_SET_VOLUME    0X06

#define CMD_SNG_CYCL_PLAY 0X08  // Single Cycle Play.
#define CMD_SEL_DEV       0X09
#define CMD_SLEEP_MODE    0X0A
#define CMD_WAKE_UP       0X0B
#define CMD_RESET         0X0C
#define CMD_PLAY          0X0D
#define CMD_PAUSE         0X0E
#define CMD_PLAY_FOLDER_FILE 0X0F

#define CMD_STOP_PLAY     0X16
#define CMD_FOLDER_CYCLE  0X17
#define CMD_SHUFFLE_PLAY  0x18
#define CMD_SET_SNGL_CYCL 0X19 // Set single cycle.

#define CMD_SET_DAC 0X1A
#define DAC_ON  0X00
#define DAC_OFF 0X01

#define CMD_PLAY_W_VOL        0X22
#define CMD_PLAYING_N         0x4C
#define CMD_QUERY_STATUS      0x42
#define CMD_QUERY_VOLUME      0x43
#define CMD_QUERY_FLDR_TRACKS 0x4e
#define CMD_QUERY_TOT_TRACKS  0x48
#define CMD_QUERY_FLDR_COUNT  0x4f
#define DEV_TF                0X02

void setup() {
  // Setup buzzers
  pinMode(buzzerOne, INPUT);
  pinMode(buzzerTwo, INPUT);

  // Setup wireless buttons
  pinMode(buttonOne, INPUT);
  pinMode(buttonTwo, INPUT);
  pinMode(buttonThree, INPUT);
  pinMode(buttonFour, INPUT);

  // Setup sirens and leds
  pinMode(sirenOne, OUTPUT);
  pinMode(sirenTwo, OUTPUT);

  // Setup serial
  Serial.begin(9600); // Used for debug
  Serial.println("Starting...");

  // Setup sound
  mp3.begin(9600); // mp3 player

  // Wait for MP3 player to boot
  delay(300);
  sendCommand(CMD_SEL_DEV, DEV_TF); // tell mp3 player to load TF card
  delay(200);
}

void loop() {
  int oneCount = 0;
  int twoCount = 0;

  for (int i = 0; i <= debounceSample; i++) {
    // Check if MP3 player has output
    if (mp3.available())
    {
      Serial.println(decodeMP3Answer());
    }
    
    buzzerOneState = digitalRead(buzzerOne);
    buzzerTwoState = digitalRead(buzzerTwo);

    buttonOneState = digitalRead(buttonOne);
    buttonTwoState = digitalRead(buttonTwo);
    buttonThreeState = digitalRead(buttonThree);
    buttonFourState = digitalRead(buttonFour);

    // Buzzer One
    if (buzzerOneState == HIGH || buttonOneState == HIGH) {
      oneCount++;

      if (buttonOneState == HIGH) {
        twoCount = 0; // give operator an advantage
      }
    }

    // Buzzer Two
    if (buzzerTwoState == HIGH || buttonTwoState == HIGH) {
      twoCount++;

      if (buttonTwoState == HIGH) {
        oneCount = 0; // give operator an advantage
      }
    }
  }

  if (
    (oneCount > debounceSize) ||
    (twoCount > debounceSize)
  ) {
    Serial.print("Buzzer one total:");
    Serial.println(oneCount);

    Serial.print("Buzzer two total:");
    Serial.println(twoCount);

    // Don't allow people to hold both buzzers
    if (oneCount != twoCount) {
      if (oneCount > twoCount) {
        oneWins();
      } else {
        twoWins();
      }
    }
  }
  
}

void doAnswerCorrect() {
  Serial.println("Playing correct answer sound");
  sendCommand(CMD_PLAY_W_INDEX, 0x0002);
  return;
}

void doAnswerWrong() {
  Serial.println("Playing wrong answer sound");
  sendCommand(CMD_PLAY_W_INDEX, 0x0003);
  return;
}

void doSiren(int siren) {
  digitalWrite(siren, HIGH);
  
  sirenStart = millis();
  sirenStop = sirenStart;
  
  while ((sirenStop - sirenStart) <= sirenMs) {
    buttonOneState = digitalRead(buttonOne);
    buttonTwoState = digitalRead(buttonTwo);
    buttonThreeState = digitalRead(buttonThree);
    buttonFourState = digitalRead(buttonFour);
    
    // Operator signals answer is correct
    if (buttonThreeState == HIGH) {
      Serial.println("Correct answer!");
      return doAnswerCorrect();
    }

    // Operator signals answer is wrong
    if (buttonFourState == HIGH) {
      Serial.println("Wrong answer!");
      return doAnswerWrong();
    }
    
    sirenStop = millis();
  }

  // timeout
  Serial.println("Done with wait loop.");
  digitalWrite(siren, LOW);
}

void oneWins() {
  Serial.println("BUZZER ONE!");
  anyWin();
  doSiren(sirenOne);
  
  resetGame();
}

void twoWins() {
  Serial.println("BUZZER TWO!");
  anyWin();
  doSiren(sirenTwo);
  
  resetGame();
}

void anyWin() {
  // Play buzzer sound
  Serial.println("Playing buzzer sound");
  sendCommand(CMD_PLAY_W_INDEX, 0x0001);
}

void resetGame() {
  digitalWrite(sirenOne, LOW);
  digitalWrite(sirenTwo, LOW);
  delay(100);
  Serial.println("Game reset...");
}

/********************************************************************************/
/*Function: sbyte2hex. Returns a byte data in HEX format.                 */
/*Parameter:- uint8_t b. Byte to convert to HEX.                                */
/*Return: String                                                                */


String sbyte2hex(uint8_t b)
{
  String shex;

  shex = "0X";

  if (b < 16) shex += "0";
  shex += String(b, HEX);
  shex += " ";
  return shex;
}

/********************************************************************************/
/*Function: Send command to the MP3                                         */
/*Parameter:-int8_t command                                                     */
/*Parameter:-int16_ dat  parameter for the command                              */

void sendCommand(int8_t command, int16_t dat)
{
  delay(20);
  Send_buf[0] = 0x7e;   //
  Send_buf[1] = 0xff;   //
  Send_buf[2] = 0x06;   // Len
  Send_buf[3] = command;//
  Send_buf[4] = 0x01;   // 0x00 NO, 0x01 feedback
  Send_buf[5] = (int8_t)(dat >> 8);  //datah
  Send_buf[6] = (int8_t)(dat);       //datal
  Send_buf[7] = 0xef;   //
  Serial.print("Sending: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    mp3.write(Send_buf[i]) ;
    Serial.print(sbyte2hex(Send_buf[i]));
  }
  Serial.println();
}

/********************************************************************************/
/*Function: sanswer. Returns a String answer from mp3 UART module.          */
/*Parameter:- uint8_t b. void.                                                  */
/*Return: String. If the answer is well formated answer.                        */

String sanswer(void)
{
  uint8_t i = 0;
  String mp3answer = "";

  // Get only 10 Bytes
  while (mp3.available() && (i < 10))
  {
    uint8_t b = mp3.read();
    ansbuf[i] = b;
    i++;

    mp3answer += sbyte2hex(b);
  }

  // if the answer format is correct.
  if ((ansbuf[0] == 0x7E) && (ansbuf[9] == 0xEF))
  {
    return mp3answer;
  }

  return "???: " + mp3answer;
}

/********************************************************************************/
/*Function decodeMP3Answer: Decode MP3 answer.                                  */
/*Parameter:-void                                                               */
/*Return: The                                                  */

String decodeMP3Answer() {
  String decodedMP3Answer = "";

  decodedMP3Answer += sanswer();

  switch (ansbuf[3]) {
    case 0x3A:
      decodedMP3Answer += " -> Memory card inserted.";
      break;

    case 0x3D:
      decodedMP3Answer += " -> Completed play num " + String(ansbuf[6], DEC);
      break;

    case 0x40:
      decodedMP3Answer += " -> Error";
      break;

    case 0x41:
      decodedMP3Answer += " -> Data recived correctly. ";
      break;

    case 0x42:
      decodedMP3Answer += " -> Status playing: " + String(ansbuf[6], DEC);
      break;

    case 0x48:
      decodedMP3Answer += " -> File count: " + String(ansbuf[6], DEC);
      break;

    case 0x4C:
      decodedMP3Answer += " -> Playing: " + String(ansbuf[6], DEC);
      break;

    case 0x4E:
      decodedMP3Answer += " -> Folder file count: " + String(ansbuf[6], DEC);
      break;

    case 0x4F:
      decodedMP3Answer += " -> Folder count: " + String(ansbuf[6], DEC);
      break;
  }

  return decodedMP3Answer;
}


