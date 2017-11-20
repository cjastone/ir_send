/*
 * IR Modulation Demonstration
 *
 * Enables sending of modulated infrared signals via an IR diode.
 * Uses hardware PWM to achieve oputput, switched by counting elapsed PWM cycles for a precise pulse count.
 *
 */

#define IR_DDR DDRD                         // data direction register for IR diode port
#define IR_PRT PORTD                        // output register for IR diode port
#define IR_PIN 3                            // port pin for IR diode
#define INPUT_PIN A0                        // arduino analog input pin for multi-button input
#define ADC_TOLERANCE 65                    // allowable tolerance above adc readings for button input

/* pulse lengths for panasonic */
#define PAN_PULSE_PREAMBLE 133              // preamble high pulse: sent at start of transmission
#define PAN_PULSE_PREAMBLE_GAP 67           // preamble low pulse (gap): sent following header mark
#define PAN_PULSE_BIT_MARK 17               // bit mark high pulse: sent at end of each bit in bit sequence
#define PAN_PULSE_HIGH 51                   // low pulse (gap): indicates 1 bit in bit sequence
#define PAN_PULSE_LOW 16                    // low pulse (gap): indicates 0 bit in bit sequence
#define PAN_SEQ_REPEAT 75                   // time in ms between sequence repeats

/* bit sequences for panasonic */
#define PAN_SEQ_LENGTH 48                   // length of panasonic sequence in bits
#define PAN_SEQ_REPEAT 75                   // time in ms between sequence repeats
#define PAN_ADDRESS 0x802002                // hex value for panasonic device address
#define PAN_POWER_SEQ 0xBD3D00              // hex value for panasonic power sequence
#define PAN_UP_SEQ 0xCA4A00                 // hex value for panasonic up sequence
#define PAN_DN_SEQ 0XCB4B00                 // hex value for panasonic down sequence
#define PAN_MENU_SEQ 0xD25200               // hex value for panasonic menu sequence
#define PAN_OK_SEQ 0xC94900                 // hex value for panasonic ok sequence
#define PAN_GUIDE_SEQ 0x0E8709              // hex value for panasonic guide sequence
#define PAN_EXIT_SEQ 0x54D400               // hex value for panasonic exit sequence
#define PAN_INPUT_SEQ 0x850500              // hex value for panasonic input sequence

void setup() {
  IR_DDR |= (1 << IR_PIN);                  // set up output for IR diode pin

  TCCR2A = 0;                               // clear TCCR2A register
  TCCR2B = 0;                               // clear TCCR2B register
  TCNT2 = 0;                                // initialise TCNT2 counter

  OCR2A = 208;                              // set compare match register A for 208 units (16 MHz / (38.4 kHz * 2))
  OCR2B = 104;                              // set compare match register B for 104 units for 50% duty cycle

  TCCR2A |= (1 << WGM20);                   // enable phase-correct PWM waveform generation
  TCCR2B |= (1 << WGM22);                   // enable OC2A output
  TCCR2B |= (1 << CS20);                    // set prescaler to 1 (no prescaling)
}

/* wait specified number of pwm cycles for pulse timing */
void wait(uint8_t cycles){
  for (uint8_t count = 0; count < cycles; count++) {
      while ((TIFR2 & (1 << OCF2A)) == 0);  // wait for output compare match flag (will be raised once per cycle)
      TIFR2 |= (1 << OCF2A);                // reset output compare match flag
  }
}

/* set next pulse to be sent*/
void send(uint8_t length) {
  TCCR2A |= (1 << COM0B1);                  // enable PWM output
  wait(length);                             // wait pulse length
  TCCR2A &= ~(1 << COM0B1);                 // disable PWM output
}

/* send specified ir sequence */
void sendSequence(uint64_t sequence) {
  /* send complete preamble sequence prior to bit sequence */
  send(PAN_PULSE_PREAMBLE);                 // send preamble pulse
  wait(PAN_PULSE_PREAMBLE_GAP);             // send preamble gap
  send(PAN_PULSE_BIT_MARK);                 // send initial bitmark pulse

  /* send complete bitstream for specified sequence and address */
  sequence = sequence << 24 | PAN_ADDRESS ; // append address to end of sequence
  for (uint8_t i = 0; i < PAN_SEQ_LENGTH; i++) {
    uint8_t pulse = sequence >> i & 1;      // extract individual bits from sequence
    if (pulse == 0) wait(PAN_PULSE_LOW);    // send 0 gap
    if (pulse == 1) wait(PAN_PULSE_HIGH);   // send 1 gap
    send(PAN_PULSE_BIT_MARK);               // send bit mark pulse
  }
}

/* read and interpret analog button input */
uint8_t readButton() {
  uint16_t reading = analogRead(INPUT_PIN);
  if (reading > 999) return 0;              // most likely result so appears first
  if (reading < ADC_TOLERANCE) return 1;
  if (reading < 127 + ADC_TOLERANCE) return 2;
  if (reading < 256 + ADC_TOLERANCE) return 3;
  if (reading < 388 + ADC_TOLERANCE) return 4;
  if (reading < 521 + ADC_TOLERANCE) return 5;
  if (reading < 650 + ADC_TOLERANCE) return 6;
  if (reading < 779 + ADC_TOLERANCE) return 7;
  if (reading < 908 + ADC_TOLERANCE) return 8;
  return 0;                                 // return no result if input cannot be interpreted
}

void loop() {
  /* read button input and send corresponding sequence */
  switch(readButton()) {
    case 8: sendSequence(PAN_INPUT_SEQ); break;
    case 7: sendSequence(PAN_POWER_SEQ); break;
    case 6: sendSequence(PAN_GUIDE_SEQ); break;
    case 5: sendSequence(PAN_MENU_SEQ); break;
    case 4: sendSequence(PAN_UP_SEQ); break;
    case 3: sendSequence(PAN_DN_SEQ); break;
    case 2: sendSequence(PAN_OK_SEQ); break;
    case 1: sendSequence(PAN_EXIT_SEQ); break;
  }
  _delay_ms(PAN_SEQ_REPEAT);
}
