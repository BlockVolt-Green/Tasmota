#ifndef MY_MODBUS_H
#define MY_MODBUS_H

// HardwareSerial Myserial(2); // define a Serial for UART2
const int MySerialRX = 16;
const int MySerialTX = 17;

/*!
  We're using a MAX485-compatible RS485 Transceiver.
  Rx/Tx is hooked up to the hardware serial port at 'Serial'.
  The Data Enable and Receiver Enable pins are hooked up as follows:
*/
#define MAX485_DE 5
// #define MUX_SEL
// #define MAX485_RE_NEG  2

extern void init_meter_modbus();
extern void fetch_meter_readings(float *readings);

#endif // MY_MODBUS_H