#include <EEPROM.h>

void interruptRoutineIncrement();
void interruptRoutineDecrement();
uint16_t readShortValue(int addr);
void writeShortValue(int addr, uint16_t val);

enum Wiring {
    CC,
    CA
};

const int storage_addr = 0;
const Wiring display = CC;
const int increment_pin = 2;
const int decrement_pin = 3;
const int reset_pin = 4;
const int output_pin = 5;

// Pins for segments a,b,c,d,e,f,g
const int segment_pins[] = {6, 7, 8, 9, 10, 11, 12};
// Pins for digits starting with the left one
const int common_pins[] = {A0, A1, A2, A3};

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