#include <Keyboard.h>

#define NSAMPLES 40     // Number of samples to take
#define COOLDOWN 10000  // Cooldown after each sample to regenerate the coil voltage
#define THRESHOLD 2000  // Time difference threshold to trigger a hammer detection

#define RXLED 17    // The on-board LED
#define COIL1 3     // The inputs corresponding to the three coils
#define COIL2 0
#define COIL3 1
#define OUT 2       // The output connected to all coils

// Port 3 is Interrupt 0, Port 1 is interrupt 2, Port 0 is interrupt 3

volatile uint8_t coil_status;
volatile uint16_t dt1, dt2, dt3;

// The sum of the reset times for each coil, and the running average of that time
uint32_t time1, time2, time3, avg1, avg2, avg3;
uint8_t countdown = 100;

// Interrupt handlers for the three coils
void coil1_handler() {
  EIMSK &= 0xfe;         // clear bit 0 to disable the interrupt
  dt1 = TCNT1;           // save the timer value to the global variable
  coil_status &= 0xfe;   // set the flag indicating that the coil is done
}

void coil2_handler() {
  EIMSK &= 0xfb; // clean bit 2
  dt2 = TCNT1;
  coil_status &= 0xfd;
}

void coil3_handler() {
  EIMSK &= 0xf7; // clear bit 3
  dt3 = TCNT1;
  coil_status &= 0xfb;
}

// Wait for the given number of CPU cycles
void delayCycles(uint16_t n) {
  TCNT1 = 0;
  while (TCNT1 < n);  
}

// Initiates a single measurement of how long it takes for each input pin to go to LOW
// after the output pin is set to HIGH and stores the result in dt1 to dt3
void sample() {
  TCNT1 = 0;              // reset timer
  coil_status = 0x07;     // reset coil status
  EIMSK |= 0x0d;          // enable all three interrupts
  digitalWrite(OUT, HIGH);  // switch on output pin and wait for all inputs to go low
  while (coil_status);
  digitalWrite(OUT, LOW);
  delayCycles(COOLDOWN);  // cooldown period to reset the coils
}

// Takes NSAMPLES many samples and returns the total time for each coil in the variables
// time1 to time3
void measure() {
  // Take dummy sample to ensure consistent coil saturation
  // (otherwise the first sample would be different from the others since
  // the coils had much more time to reset)
  sample();
  
  time1 = 0;
  time2 = 0;
  time3 = 0;
  for (uint8_t i = 0; i < NSAMPLES; i++) {
    sample();
    time1 += (uint32_t) dt1;
    time2 += (uint32_t) dt2;
    time3 += (uint32_t) dt3;
  }
}

// Send Ctrl+Shift+Alt+S, then send Ctrl+Shift+Cmd+S (for MacOS, then send a reutrn
// This needs to be configured as the sledgehammer panel shortcut in Isabelle/JEdit
void fire () {
  Keyboard.press(KEY_LEFT_CTRL);
  Keyboard.press(KEY_LEFT_SHIFT);
  Keyboard.press(KEY_LEFT_ALT);
  Keyboard.press('S');
  delay(100);
  Keyboard.releaseAll();
  
  Keyboard.press(KEY_LEFT_CTRL);
  Keyboard.press(KEY_LEFT_SHIFT);
  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press('S');
  delay(100);
  Keyboard.releaseAll();

  Keyboard.press(KEY_RETURN);
  delay(100);
  Keyboard.releaseAll();
}


// Do initial setup
void setup() {
  pinMode(OUT, OUTPUT);
  pinMode(COIL1, INPUT);
  pinMode(COIL2, INPUT);
  pinMode(COIL3, INPUT);
  pinMode(RXLED, OUTPUT);

  // Some magic to set up the 16 bit timer to count cycle-precise (i.e. steps of 62.5 ns)
  TCCR1A = 0;
  TCCR1B = 1;
  TCCR1C = 0;

  attachInterrupt(digitalPinToInterrupt(COIL1), coil1_handler, FALLING);
  attachInterrupt(digitalPinToInterrupt(COIL2), coil2_handler, FALLING);
  attachInterrupt(digitalPinToInterrupt(COIL3), coil3_handler, FALLING);
  digitalWrite(RXLED, HIGH);
  Keyboard.begin();
}


// Main loop
void loop() {
  // Measure and compute sum of differences to running average
  measure();
  int32_t diff = 0;
  int32_t tmp;
  tmp = (int32_t) time1 - (int32_t) avg1;
  if (tmp > 0) diff += (uint32_t) (tmp);
  tmp = (int32_t) time2 - (int32_t) avg2;
  if (tmp > 0) diff += (uint32_t) (tmp);
  tmp = (int32_t) time3 - (int32_t) avg3;
  if (tmp > 0) diff += (uint32_t) (tmp);

  // Fire if difference exceeds threshold
  bool b = diff > THRESHOLD;

  // Factor in the new measurement into the running average
  // This causes a slow attenuation after each change
  avg1 = (9 * avg1 + time1) / 10;
  avg2 = (9 * avg2 + time2) / 10;
  avg3 = (9 * avg3 + time3) / 10;

  // The countdown variable specifies how many of the next samples
  // will be ignored (i.e. not fire).
  if (countdown == 0) {
    if (b) {
      digitalWrite(RXLED, LOW);     // For some reason, "LOW" means that the LED is on
      fire();
      countdown = 100;              // Ignore the next 100 measurements to allow for attenuation
                                    // of the running average. This can be decrease significantly,
                                    // but it may be useful to have a "cooldown period" of a few
                                    // seconds where the hammer will not fire again.
    } else {
      digitalWrite(RXLED, HIGH);
    }
  } else {
    countdown--;
  }

  // Wait for 10 ms because of reasons
  delay(10);
}

