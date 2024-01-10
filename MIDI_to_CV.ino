#include <USBHost_t36.h>
#include <Adafruit_MCP4728.h>
#include <Wire.h>

USBHost myusb;
USBHub hub1(myusb);
MIDIDevice midi1(myusb);
Adafruit_MCP4728 mcp;

const float VOLT_PER_OCTAVE = 1.0; // 1V/octave standard
const float voltaje_ref = 5.0;
const int mono_switch = 1; // Digital pin connected to the SPDT switch
const int vel_switch = 2; // Digital pin connected to the SPDT switch
const int gate1 = 14;
const int gate2 = 41;
const int gate3 = 40;

bool isMonophonic = false; // Variable to track the current configuration
bool isVel = false; // Variable to track the current configuration
unsigned long lastDebounceTime_mono = 0;
unsigned long lastDebounceTime_vel = 0;
unsigned long debounceDelay_mono = 50;
unsigned long debounceDelay_vel = 50;
int lastswitchState_mono = HIGH;
int lastswitchState_vel = HIGH;
int switchState = HIGH;
int switchState_vel = HIGH;

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

  pinMode(mono_switch, INPUT_PULLUP); // Set the switch pin as input with pull-up
  pinMode(vel_switch, INPUT_PULLUP); // Set the switch pin as input with pull-up
  digitalWrite(gate1, HIGH); //Gate 1
  digitalWrite(gate2, HIGH); //Gate 2
  digitalWrite(gate3, HIGH); //Gate 3
}

void loop() {
  myusb.Task();
  midi1.read();

  int switchReading = digitalRead(mono_switch);

  if (switchReading != lastswitchState_mono) {
    lastDebounceTime_mono = millis();
  }

  if ((millis() - lastDebounceTime_mono) > debounceDelay_mono) {
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

  lastswitchState_mono = switchReading;

  int switchReading_vel = digitalRead(vel_switch);

  if (switchReading_vel != lastswitchState_vel) {
    lastDebounceTime_vel = millis();
  }

  if ((millis() - lastDebounceTime_vel) > debounceDelay_vel) {
    if (switchReading_vel != switchState_vel) {
      switchState_vel = switchReading_vel;

      if (switchState_vel == LOW) {
        isVel = true;
        Serial.println("Switched to Velocity Mode");
      } else {
        isVel = false;
        Serial.println("Switched to Control Change Mode");
      }
    }
  }

  lastswitchState_vel = switchReading_vel;
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

    float voltage_velocity = velocity / 127.0 * voltaje_ref; // Map value to voltage between 0 and 5V
    uint16_t dacValue = voltage * 4095 / voltaje_ref; 
    uint16_t dacValue_velocity = voltage_velocity * 4095 / voltaje_ref; 

    mcp.setChannelValue(MCP4728_CHANNEL_A, dacValue);
    mcp.setChannelValue(MCP4728_CHANNEL_B, dacValue);
    mcp.setChannelValue(MCP4728_CHANNEL_C, dacValue);

    if(isVel){
      mcp.setChannelValue(MCP4728_CHANNEL_D, dacValue_velocity);
    }

    //Gate 
    digitalWrite(gate1, LOW); //Gate 1
    digitalWrite(gate2, LOW); //Gate 2
    digitalWrite(gate3, LOW); //Gate 3

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
        if(velocity != 0){
          switch (i) {
            case 0:
              digitalWrite(gate1, LOW); //Gate 1
              break;
            case 1:
              digitalWrite(gate2, LOW); //Gate 1
              break;
            case 2:
              digitalWrite(gate3, LOW); //Gate 1
              break;
              }
          }

        if(isVel){
          float voltage_velocity = velocity / 127.0 * voltaje_ref; // Map value to voltage between 0 and 5V
          uint16_t dacValue_velocity = voltage_velocity * 4095 / voltaje_ref; 
          mcp.setChannelValue(MCP4728_CHANNEL_D, dacValue_velocity);
          }
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

    //Gate 
    digitalWrite(gate1, HIGH); //Gate 1
    digitalWrite(gate2, HIGH); //Gate 2
    digitalWrite(gate3, HIGH); //Gate 3

    if(isVel){
      float voltage_velocity = velocity / 127.0 * voltaje_ref; // Map value to voltage between 0 and 5V
      uint16_t dacValue_velocity = voltage_velocity * 4095 / voltaje_ref; 
      mcp.setChannelValue(MCP4728_CHANNEL_D, dacValue_velocity);
    }

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
        if(velocity == 0){
          switch (i) {
            case 0:
              digitalWrite(gate1, HIGH); //Gate 1
              break;
            case 1:
              digitalWrite(gate2, HIGH); //Gate 1
              break;
            case 2:
              digitalWrite(gate3, HIGH); //Gate 1
              break;
          }
      }

        if(isVel){
        float voltage_velocity = velocity / 127.0 * voltaje_ref; // Map value to voltage between 0 and 5V
        uint16_t dacValue_velocity = voltage_velocity * 4095 / voltaje_ref; 
        mcp.setChannelValue(MCP4728_CHANNEL_D, dacValue_velocity);
        }
        break;
      }
    }
  }
}

void updateOutputs() {
  for (int i = 0; i < 3; i++) {
    if (notes[i] != 127) {
      float voltage = (notes[i] - 24) / 12.0 * VOLT_PER_OCTAVE;
      uint16_t dacValue = voltage * 4095 / voltaje_ref;

      switch (i) {
        case 0:
          mcp.setChannelValue(MCP4728_CHANNEL_A, dacValue);
          break;
        case 1:
          mcp.setChannelValue(MCP4728_CHANNEL_B, dacValue);
          break;
        case 2:
          mcp.setChannelValue(MCP4728_CHANNEL_C, dacValue);
          break;
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

  if(isVel == false){
    mcp.setChannelValue(MCP4728_CHANNEL_D, dacValue);
  }
  
}
