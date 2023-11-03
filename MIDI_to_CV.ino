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

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int lastSwitchState = HIGH;
int switchState = HIGH;

byte notes[3] = {127, 127, 127}; // Array to store the notes for polyphonic mode

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
  digitalWrite(41, HIGH);
}

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

void myNoteOn(byte channel, byte note, byte velocity) {
  if (isMonophonic) {
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
    mcp.setChannelValue(MCP4728_CHANNEL_B, dacValue);
    mcp.setChannelValue(MCP4728_CHANNEL_C, dacValue);

    //Gate 
    digitalWrite(41, LOW);

  } 
  
  else {
    float voltage = (note - 24) / 12.0 * VOLT_PER_OCTAVE; // Shift down by one octave
    Serial.print("Note On, ch=");
    Serial.print(channel, DEC);
    Serial.print(", note=");
    Serial.print(note, DEC);
    Serial.print(", velocity=");
    Serial.print(velocity, DEC);
    Serial.print(", voltage=");
    Serial.println(voltage, 3);
    // Polyphonic mode logic
    for (int i = 0; i < 3; i++) {
      if (notes[i] == 127) {
        notes[i] = note;
        updateOutputs();
        break;
      }
    }
  }
}

void myNoteOff(byte channel, byte note, byte velocity) {
  if (isMonophonic) {
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

    digitalWrite(41, HIGH);
  }

  else{
      for (int i = 0; i < 3; i++) {
        float voltage = (note - 24) / 12.0 * VOLT_PER_OCTAVE; // Shift down by one octave
        Serial.print("Note Off, ch=");
        Serial.print(channel, DEC);
        Serial.print(", note=");
        Serial.print(note, DEC);
        Serial.print(", velocity=");
        Serial.print(velocity, DEC);
        Serial.print(", voltage=");
        Serial.println(voltage, 3);

      if (notes[i] == note) {
        notes[i] = 127;
        updateOutputs();
        break;
      }
    }
  }
}

void updateOutputs() {
  for (int i = 0; i < 3; i++) {
    if (i == 0){
      if (notes[i] != 127) {
      float voltage = (notes[i] - 24) / 12.0 * VOLT_PER_OCTAVE;
      uint16_t dacValue = voltage * 4095 / voltaje_ref;
      mcp.setChannelValue(MCP4728_CHANNEL_A, dacValue); // Output the note to the corresponding channel
      }
    }

    if (i == 1){
      if (notes[i] != 127) {
      float voltage = (notes[i] - 24) / 12.0 * VOLT_PER_OCTAVE;
      uint16_t dacValue = voltage * 4095 / voltaje_ref;
      mcp.setChannelValue(MCP4728_CHANNEL_B, dacValue); // Output the note to the corresponding channel
      }

    }

    if (i == 2){
      if (notes[i] != 127) {
      float voltage = (notes[i] - 24) / 12.0 * VOLT_PER_OCTAVE;
      uint16_t dacValue = voltage * 4095 / voltaje_ref;
      mcp.setChannelValue(MCP4728_CHANNEL_C, dacValue); // Output the note to the corresponding channel
      }
    }
  }
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
