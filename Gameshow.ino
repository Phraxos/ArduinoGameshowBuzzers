const int buzzerOne = 12;
const int buzzerTwo = 13;

const int buttonOne = 8;
const int buttonTwo = 9;
const int buttonThree = 10;
const int buttonFour = 11;

const int debounceSample = 20;
const int debounceSize = 15;

const unsigned long sirenMs = 6000; // how long to run siren
unsigned long sirenStart;
unsigned long sirenStop;


int buzzerOneState;
int buzzerTwoState;

int buttonOneState;
int buttonTwoState;
int buttonThreeState;
int buttonFourState;

int sirenOne = 6;
int sirenTwo = 7;

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
}

void loop() {
  int oneCount = 0;
  int twoCount = 0;

  for (int i = 0; i <= debounceSample; i++) {
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
    Serial.print("One total:");
    Serial.println(oneCount);

    Serial.print("Two total:");
    Serial.println(twoCount);

    // Don't allow people to hold the buttons
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
  return;
}

void doAnswerWrong() {
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

}

void resetGame() {
  digitalWrite(sirenOne, LOW);
  digitalWrite(sirenTwo, LOW);
  delay(100);
  Serial.println("Game reset...");
}


