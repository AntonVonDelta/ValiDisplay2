#include <EEPROM.h>

#define SZ(x) sizeof(x) / sizeof(x[0])

// For both wiring configurations use NPN transistors!
// This code won't work with PNP transistors without some adjustments.
enum Wiring {
    CC,
    CA
};

void showDisplay(uint16_t nr);
void interruptRoutineIncrement();
void interruptRoutineDecrement();
uint16_t readShortValue(int addr);
void writeShortValue(int addr, uint16_t val);

const Wiring display = Wiring::CA;
const int led_on_state = (display == Wiring::CA ? LOW : HIGH);
const int led_off_state = (display == Wiring::CA ? HIGH : LOW);
const int refresh_period_ms = 20; // Period in ms of refresh frequency
const int storage_addr = 0;
const int increment_pin = 2;
const int decrement_pin = 3;
const int reset_pin = 4;
const int output_pin = 5;

// Pins for segments a,b,c,d,e,f,g
const int segment_pins[] = {6, 7, 8, 9, 10, 11, 12};
// Pins for digits starting with the left one
const int common_pins[] = {A0, A1, A2, A3};
// The index of the array is the digit 0-9
// The value is 7 bits with the lowest one being a then b, etc.
// MSB -->  0000 0000 <-- LSB
//           gfe dcba
const uint8_t map_digit_to_segments[] = {0x3F, 0x6, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x7, 0x7F, 0x6F};

// Use values of 1 byte otherwise interrupts can alter the value mid-reading
// https://www.arduino.cc/reference/en/language/variables/variable-scope-qualifiers/volatile/
volatile uint8_t increment = 0;
volatile uint8_t decrement = 0;
int16_t counter = 0;

void setup() {
    Serial.begin(9600);

    counter = readShortValue(storage_addr);

    pinMode(increment_pin, INPUT_PULLUP);
    pinMode(decrement_pin, INPUT_PULLUP);
    pinMode(reset_pin, INPUT_PULLUP);
    pinMode(output_pin, OUTPUT);

    digitalWrite(output_pin, HIGH);

    for (int i = 0; i < SZ(segment_pins); i++) {
        pinMode(segment_pins[i], OUTPUT);
        digitalWrite(segment_pins[i], led_off_state);
    }
    for (int i = 0; i < SZ(common_pins); i++) {
        pinMode(common_pins[i], OUTPUT);
        digitalWrite(common_pins[i], LOW);
    }

    attachInterrupt(digitalPinToInterrupt(increment_pin), interruptRoutineIncrement, RISING);
    attachInterrupt(digitalPinToInterrupt(decrement_pin), interruptRoutineDecrement, RISING);
}

void loop() {
    int prev_increment = 0;
    int prev_decrement = 0;

    if (digitalRead(reset_pin) == LOW) {
        counter = 0;
        writeShortValue(storage_addr, 0);
    }

    noInterrupts();
    prev_increment = increment;
    prev_decrement = decrement;
    increment = 0;
    decrement = 0;
    interrupts();

    if (prev_decrement && prev_decrement >= counter) {
        // Means that the counter was moved and it passed through 0.
        digitalWrite(output_pin, LOW);
        delayMicroseconds(1); // Most chips need at least 20ns so this is enough.
        digitalWrite(output_pin, HIGH);
        delayMicroseconds(1);
    }

    if (prev_increment && prev_increment >= 10000 - counter) {
        // Means that the counter was moved and it passed through 0.
        digitalWrite(output_pin, LOW);
        delayMicroseconds(1); // Most chips need at least 20ns so this is enough.
        digitalWrite(output_pin, HIGH);
        delayMicroseconds(1);
    }

    counter += prev_increment;
    counter %= 10000;

    counter = (counter - prev_decrement) + 10000;
    counter %= 10000;

    writeShortValue(storage_addr, counter);
    showDisplay(counter);
}

void showDisplay(uint16_t nr) {
    int digit_period_ms = refresh_period_ms / SZ(common_pins);      // Time to spend on each digit
    bool show_digits = false;                                        // Do not show the far left digits if they are 0

    for (int i = SZ(common_pins) - 1; i >= 0; i--) {
        uint8_t digit = (nr / round(pow(10, i))) % 10;
        uint8_t active_segments = map_digit_to_segments[digit];

        if (i == SZ(common_pins) - 1)
            digitalWrite(common_pins[0], LOW);
        else
            digitalWrite(common_pins[i + 1], LOW);

        if (!show_digits && digit == 0 && i != 0) {
            // We want to control the total timings very precisely
            // Otherwise if we skip the delay for some digits then
            // the overall panel will look brighter.
            // We don't want the brightness to vary depending on the number if digits shown.
            delay(digit_period_ms);
            continue;
        }
        show_digits = true;

        // Start iterating through leds: a to g
        for (int j = 0; j < SZ(segment_pins); j++) {
            // Check if the bit corresponding to led with index <j> is on
            if ((active_segments >> j) & 0x1) {
                digitalWrite(segment_pins[j], led_on_state);
            } else {
                digitalWrite(segment_pins[j], led_off_state);
            }
        }

        digitalWrite(common_pins[i], HIGH);
        delay(digit_period_ms);
    }
}

void interruptRoutineIncrement() {
    static int signal_counter = 0;
    signal_counter++;

    if (signal_counter == 18) {
        signal_counter = 0;
        increment++;
    }
}

void interruptRoutineDecrement() {
    static int signal_counter = 0;
    signal_counter++;

    if (signal_counter == 18) {
        signal_counter = 0;
        decrement++;
    }
}

uint16_t readShortValue(int addr) {
    return (EEPROM.read(addr + 1) << 8) + EEPROM.read(addr);
}
void writeShortValue(int addr, uint16_t val) {
    EEPROM.update(addr, val & 0xFF);
    EEPROM.update(addr + 1, val >> 8);
}