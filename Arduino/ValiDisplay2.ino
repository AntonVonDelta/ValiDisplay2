#include <EEPROM.h>

#define SZ(x) sizeof(x) / sizeof(x[0])

void showDisplay(uint16_t nr);
void interruptRoutineIncrement();
void interruptRoutineDecrement();
uint16_t readShortValue(int addr);
void writeShortValue(int addr, uint16_t val);

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
// 0000 0000
//  gfe dcba
const uint8_t map_digit_to_pins[] = {0x3F, 0x6, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x7, 0x7F, 0x6F};

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

        // Common-Anode segments have inverted logic.
        // Set to low to disable
        digitalWrite(segment_pins[i], HIGH);
    }
    for (int i = 0; i < SZ(common_pins); i++) {
        pinMode(common_pins[i], OUTPUT);
        digitalWrite(common_pins[i], LOW);
    }

    attachInterrupt(digitalPinToInterrupt(increment_pin), interruptRoutineIncrement, RISING);
    attachInterrupt(digitalPinToInterrupt(decrement_pin), interruptRoutineDecrement, RISING);
}

void loop() {
    if (digitalRead(reset_pin) == LOW) {
        counter = 0;
        writeShortValue(storage_addr, 0);
    }

    if (decrement && decrement >= counter + increment) {
        // Means the counter has passed the 0 mark
        // Vague situation:
        //  What should happen if counter = 0 and +1 is added and -1 at the same time?
        //  Is it that the counter increased and then decreased to 0 triggerin the output?
        //  Or is it that the counter decreased to 9999 and then back to 0?
        digitalWrite(output_pin, LOW);
    } else {
        digitalWrite(output_pin, HIGH);
    }

    counter += increment;
    counter %= 10000;
    increment = 0;

    counter = (counter - decrement) + 10000;
    counter %= 10000;
    decrement = 0;

    showDisplay(counter);
}

void showDisplay(uint16_t nr) {
    for (int i = 0; i < 4; i++) {
        uint8_t digit = nr % 10;
        uint8_t active_pins = map_digit_to_pins[digit];

        if (i == 0)
            digitalWrite(common_pins[3], LOW);
        else
            digitalWrite(common_pins[i - 1], LOW);

        // Start iterating through leds: a to g
        for (int j = 0; j < 7; j++) {
            // Check if the bit corresponding to led with index <j> is on
            if ((active_pins >> j) & 0x1) {
                digitalWrite(segment_pins[j], HIGH);
            } else {
                digitalWrite(segment_pins[j], LOW);
            }
        }
        digitalWrite(common_pins[i], HIGH);

        nr /= 10;
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