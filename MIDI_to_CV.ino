#include <USBHost_t36.h>
#include <Adafruit_MCP4728.h>
#include <Wire.h>

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
MIDIDevice midi1(myusb);
Adafruit_MCP4728 mcp;

const float VOLT_PER_OCTAVE = 1.0; // 1V/octave standard
const float voltaje_ref = 5.0;
const int switchPin = 2; // Digital pin connected to the SPDT switch

bool isMonophonic = false; // Variable to track the current configuration

bool previousSwitchState = HIGH; // Variable to track the previous state of the switch

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("Ejemplo MIDI Input I2C");
  delay(10);
  myusb.begin();

  midi1.setHandleNoteOn(myNoteOn);
  midi1.setHandleNoteOff(myNoteOff);
  midi1.setHandleControlChange(myControlChange);

  // Try to initialize!
  if (!mcp.begin(0x64)) {
    Serial.println("Failed to find MCP4728 chip");
    while (1) {
      delay(10);
    }
  }
  else{
    Serial.println("MCP4728 encontrado");
  }

  pinMode(switchPin, INPUT_PULLUP); // Set the switch pin as input with pull-up
}

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int lastSwitchState = HIGH;
int switchState = HIGH;

void loop() {
  myusb.Task();
  midi1.read();

  int switchReading = digitalRead(switchPin);

  if (switchReading != lastSwitchState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (switchReading != switchState) {
      switchState = switchReading;

      if (switchState == LOW) {
        isMonophonic = true;
        Serial.println("Switched to Monophonic Mode");
      } else {
        isMonophonic = false;
        Serial.println("Switched to Polyphonic Mode");
      }
    }
  }

  lastSwitchState = switchReading;
}



// ... (Rest of your code remains the same)

void myNoteOn(byte channel, byte note, byte velocity) {
  float voltage = (note - 24) / 12.0 * VOLT_PER_OCTAVE; // Shift down by one octave

  Serial.print("Note On, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.print(velocity, DEC);
  Serial.print(", voltage=");
  Serial.println(voltage, 3);
  
  uint16_t dacValue = voltage * 4095 / voltaje_ref; 
  mcp.setChannelValue(MCP4728_CHANNEL_A, dacValue);

}

void myNoteOff(byte channel, byte note, byte velocity) {
  float voltage = (note - 24) / 12.0 * VOLT_PER_OCTAVE; // Shift down by one octave

  Serial.print("Note Off, ch=");
  Serial.print(channel, DEC);
  Serial.print(", note=");
  Serial.print(note, DEC);
  Serial.print(", velocity=");
  Serial.print(velocity, DEC);
  Serial.print(", voltage=");
  Serial.println(voltage, 3);

  uint16_t dacValue = voltage * 4095 / voltaje_ref;
  mcp.setChannelValue(MCP4728_CHANNEL_A, dacValue);
  mcp.setChannelValue(MCP4728_CHANNEL_B, dacValue);
  mcp.setChannelValue(MCP4728_CHANNEL_C, dacValue);
}

void myControlChange(byte channel, byte control, byte value) {
  float voltage = value / 127.0 * voltaje_ref; // Map value to voltage between 0 and 5V

  Serial.print("Control Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", control=");
  Serial.print(control, DEC);
  Serial.print(", value=");
  Serial.print(value, DEC);
  Serial.print(", voltage=");
  Serial.println(voltage, 3);

  uint16_t dacValue = voltage * 4095 / voltaje_ref;
  mcp.setChannelValue(MCP4728_CHANNEL_D, dacValue);
}

