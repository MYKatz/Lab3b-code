/* LED array test code
 *
 * Reads (x,y) co-ordinates from the Serial Monitor and toggles the state of
 * the LED at that co-ordinate. The co-ordinates are specified as "x y", e.g.
 * "1 2", followed by a newline. Invalid co-ordinates are rejected.
 *
 * You need to fill in all the places marked TODO.
 *
 * == Setting up the Serial Monitor ==
 * The Serial Monitor must be configured (bottom-right corner of the screen) as:
 *   - Newline (for the line ending)
 *   - Baud rate 115200
 *
 * ENGR 40M
 * July 2018
 */

// Arrays of pin numbers. Fill these in with the pins to which you connected
// your anode (+) wires and cathode (-) wires.
const byte ANODE_PINS[8] = {13, 12, 11, 10, 9, 8, 7, 6};
const byte CATHODE_PINS[8] = {A3, A2, A1, A0, 5, 4, 3, 2};
const byte BUTTON_PIN = A5;

const unsigned long DEBOUNCE_TIME = 50;
const unsigned long PIPE_TIME = 1000;

/*
 * Digit displays from one to nine
 * Assumes a 3 (width) by 6 display
 * Uses lowest 18 bits
 * Starts top left, goes row by row
 */
const long DIGIT_DISPLAYS[10] = {
  0b111101101101101111,
  0b010010010010010010,
  0b111001001010101111,
  0b111100100111100111,
  0b100100100111101101,
  0b111100100111001111,
  0b111101101111001111,
  0b100100100100100111,
  0b111101111111101111,
  0b100100100111101111,
};


void setup() {

  for (byte i = 0; i < 8; i++) {
    pinMode(ANODE_PINS[i], OUTPUT);
    pinMode(CATHODE_PINS[i], OUTPUT);
  }

  for (byte i = 0; i < 8; i++) {
    // Disable all pins
    digitalWrite(ANODE_PINS[i], HIGH);
    digitalWrite(CATHODE_PINS[i], HIGH);
  }

  // Button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize serial communication
  // (to be read by Serial Monitor on your computer)
  Serial.begin(115200);
  Serial.setTimeout(100);
}

void display(byte pattern[8][8]) {
  for (byte i = 0; i < 8; i++) {
    digitalWrite(ANODE_PINS[i], LOW);
    for (byte j = 0; j < 8; j++) {
      if(pattern[i][j] > 0) {
        digitalWrite(CATHODE_PINS[j], LOW);
        delayMicroseconds(10 * pattern[i][j]);
        digitalWrite(CATHODE_PINS[j], HIGH); 
      }
    }
    digitalWrite(ANODE_PINS[i], HIGH);
  }
}

void clear(byte pattern[8][8]) {
  for (byte i = 0; i < 8; i++) {
    for (byte j = 0; j < 8; j++) {
     pattern[i][j] = 0; 
    }
  }
}

void displayScore(byte pattern[8][8], int score) {
  score = score % 100; // ensure 1-2 digits
  drawDigit(pattern, 1, 0, score / 10); 
  drawDigit(pattern, 1, 4, score % 10);
}

// Draws a 3x6 digit on the display
// num must be from 0-9;
void drawDigit(byte pattern[8][8], int originY, int originX, int num) {
  long digitByte = DIGIT_DISPLAYS[num];
  for(int i = 0; i < 18; i++) {
    int val = bitRead(digitByte, i);
    int x = originX + i%3;
    int y = originY + i/3;
    if(val == 1) {
      Serial.print(x);
      Serial.print(",");
      Serial.println(y);
    }
    pattern[y][x] = val * 5;
  }
}

void setPipes(byte pattern[8][8], int pipes[8]) {
  for(int i = 0; i < 8; i++) {
    if(pipes[i] > 0) {
      for(int j = 0; j < 8; j++) {
        if(j < pipes[i] - 1 || j > pipes[i] + 1) pattern[j][i] = 20;
      }
    }
  }
}

void clearPipes(int pipes[8]) {
  for(int i = 0; i < 8; i++) {
    pipes[i] = 0;
  }
}

enum GameState {
  Stopped,
  Playing,
  Score,
};

void loop() {


  // use 'static' so that it retains its value between successive calls of loop()
  static byte ledOn[8][8];
  static unsigned long lastLoop = millis();
  static unsigned long lastRead = millis();
  static unsigned long lastPipeMove = millis();
  static int lastButtonState = LOW;
  static GameState state = Stopped;

  // Variable, to simulate gravity
  static unsigned long timePerFallLoop = 300;

  // Game vars
  static byte pos = 0;
  static int pipes[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // valid pipes are from 1-6, 0 is none. Number is "center" of opening (+/- 1)
  static int score = 0;

  // test code for score display, uncomment to use
  /*
  state = Score;
  score = 99;
  */

  if(state == Stopped) {
    int reading = digitalRead(BUTTON_PIN);
    if (reading == LOW) {
      state = Playing;
      score = 0;
    }
  }
  else if(state == Playing) {
    clear(ledOn);
    unsigned long time = millis();
  
    int reading = digitalRead(BUTTON_PIN);
    if (reading == LOW && time - lastRead > DEBOUNCE_TIME && reading != lastButtonState) {
        pos = (pos - 1);
        lastLoop = time;
        timePerFallLoop = 300;
    }
    else if(time - lastLoop > timePerFallLoop) {
      pos = (pos + 1);
      lastLoop = time;
      timePerFallLoop = max(timePerFallLoop - 100, 100);
    }
    lastButtonState = reading;

    // Game over, show score
    if(pos < 0 || pos >= 8) {
      state = Score;
      delay(500); // hacky way to blink for half a second
    }
    // Also game over -- hit a pipe!
    if(pipes[0] != 0 && (pos > pipes[0] + 1 || pos < pipes[0] - 1)) {
      state = Score;
      delay(500); // hacky way to blink for half a second
    }

    if(time - lastPipeMove > PIPE_TIME) {
      int lastPipe = 0;
      // passed a pipe, add to score
      if(pipes[0] != 0) {
        score += 1;
      }
      for(int i = 0; i < 7; i++) {
        pipes[i] = pipes[i + 1];
        if(pipes[i] != 0) {
          lastPipe = i;
        }
      }
      pipes[7] = 0;
      if(lastPipe <= 4) {
        pipes[7] = int(random(1, 7)); // (inclusive, exclusive)
      }
      lastPipeMove = time;
    }

    setPipes(ledOn, pipes);
    
    // Set bird light
    ledOn[pos][0] = 20;
  }
  else if(state == Score) {
    clear(ledOn);

    displayScore(ledOn, score);

    int reading = digitalRead(BUTTON_PIN);
    if (reading == LOW) {
      state = Playing;
      score = 0;
      pos = 0;
      clearPipes(pipes);
    }
  }

  // This function gets called every loop
  display(ledOn);
}
