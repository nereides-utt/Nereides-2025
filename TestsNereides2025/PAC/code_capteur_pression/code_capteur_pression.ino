//Code acquisitoin et envoie de données capteur pression

#include <HardwareSerial.h>
#include <ModbusMaster.h>
#include <CAN.h> // ESP32 CAN library

// --- RS485 Configuration ---
HardwareSerial rs485Serial(1); // Use UART1 (you might need to adjust pins)
const int DE_RE_PIN = 16;     // Data Enable/Receive Enable pin for RS485

// --- Modbus Configuration ---
ModbusMaster node;
const byte slaveAddress = 1; // Replace with the actual slave address
const uint16_t pressureRegister = 0x0001; // Replace with the actual pressure register
const int baudRateRS485 = 9600;   // Replace with the actual baud rate
const byte parityRS485 = SERIAL_NONE; // Replace with the actual parity
const byte dataBitsRS485 = 8;
const byte stopBitsRS485 = 1;

// --- CAN Bus Configuration ---
const int canRxPin = 5;  // Example RX pin (adjust as needed)
const int canTxPin = 4;  // Example TX pin (adjust as needed)
const long canBitRate = 125E3; // Example CAN bit rate (adjust as needed)

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 #1 - Data Acquisition and CAN Sender");

  // Initialize RS485
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW); // Enable receiver by default
  rs485Serial.begin(baudRateRS485, parityRS485, dataBitsRS485, stopBitsRS485, 16, 17); // RX pin 17, TX pin 16 (adjust if needed)

  // Initialize Modbus
  node.begin(slaveAddress, rs485Serial);

  // Initialize CAN Bus
  Serial.print("Starting CAN...");
  if (CAN.begin(canBitRate, canRxPin, canTxPin)) {
    Serial.println("CAN OK!");
  } else {
    Serial.println("CAN init failed!");
    while (1);
  }
}

void loop() {
  float pressureValue = readPressureModbus();

  if (!isnan(pressureValue)) {
    Serial.print("Pressure Read: ");
    Serial.println(pressureValue);

    // Send pressure data over CAN bus
    sendPressureCAN(pressureValue);
  } else {
    Serial.println("Failed to read pressure via Modbus.");
  }

  delay(1000); // Read and send data every 1 second (adjust as needed)
}

float readPressureModbus() {
  digitalWrite(DE_RE_PIN, HIGH); // Enable RS485 transmitter
  delay(10); // Short delay for transmitter enable

  ModbusError err = node.readHoldingRegisters(pressureRegister, 2); // Assuming pressure is 2 registers (adjust based on data format)
  float pressure = NAN;

  if (err == MB_SUCCESS) {
    // Process the Modbus response based on the 23SX-H2's data format
    // This is a placeholder - you'll need to adapt this based on the datasheet
    uint16_t rawValue1 = node.getResponseBuffer(0);
    uint16_t rawValue2 = node.getResponseBuffer(1);

    // Example interpretation (adjust based on your sensor's data format):
    // pressure = (float)(rawValue1 << 16 | rawValue2) / 100.0; // Example for a combined 32-bit integer

    // **IMPORTANT: Consult the 23SX-H2 Modbus documentation to correctly interpret the raw values into pressure.**
    Serial.print("Raw Modbus Values: ");
    Serial.print(rawValue1, HEX);
    Serial.print(" ");
    Serial.println(rawValue2, HEX);
  } else {
    Serial.print("Modbus Error: ");
    Serial.println(err);
  }

  digitalWrite(DE_RE_PIN, LOW); // Enable RS485 receiver
  delay(10); // Short delay for receiver enable
  return pressure;
}

void sendPressureCAN(float pressure) {
  unsigned long canId = 0x200; // Example CAN ID for pressure data
  byte canData[4];

  // Convert float to 4 bytes (you might need to adjust endianness)
  memcpy(canData, &pressure, 4);

  CAN.beginPacket(canId);
  CAN.write(canData, 4);
  CAN.endPacket();

  Serial.print("Sent pressure (");
  Serial.print(pressure);
  Serial.println(") over CAN bus.");
}
