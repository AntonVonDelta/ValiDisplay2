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

    attachInterrupt(digitalPinToInterrupt(increment_pin), interruptRoutineIncrement, RISING);
    attachInterrupt(digitalPinToInterrupt(decrement_pin), interruptRoutineDecrement, RISING);
}

void loop() {
    if (digitalRead(reset_pin) == LOW) {
        counter = 0;
        writeShortValue(storage_addr, 0);
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