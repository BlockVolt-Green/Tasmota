#include <Arduino.h>
#include <ModbusMaster.h>
#include "include/meter_modbus.h"
#include <HardwareSerial.h>


HardwareSerial Myserial(1); // define a Serial for UART1

ModbusMaster node_v, node_i, node_pf, node_kw, node_kva;


void preTransmission()
{
  // digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  // digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

void init_meter_modbus()
{

  Serial.println("Modbus Init started");
  pinMode(MUX_SEL, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MUX_SEL , HIGH);

  Myserial.begin(9600, SERIAL_8N1, MySerialRX, MySerialTX);

  // Modbus slave ID 1
  node_v.begin(1, Myserial);
  node_i.begin(1, Myserial);
  node_pf.begin(1, Myserial);
  node_kw.begin(1, Myserial);
  node_kva.begin(1, Myserial);

  // Callbacks allow us to configure the RS485 transceiver correctly
  node_v.preTransmission(preTransmission);
  node_v.postTransmission(postTransmission);

  node_i.preTransmission(preTransmission);
  node_i.postTransmission(postTransmission);

  node_pf.preTransmission(preTransmission);
  node_pf.postTransmission(postTransmission);

  node_kw.preTransmission(preTransmission);
  node_kw.postTransmission(postTransmission);

  node_kva.preTransmission(preTransmission);
  node_kva.postTransmission(postTransmission);
  Serial.println("Modbus Init Completed");

}

void fetch_meter_readings(float *readings)
{

  uint8_t result_v, result_i, result_pf, result_kw, result_kva = 0;

  uint32_t num = 0;
  float f = 0;

  // Read 16 registers starting at 0x3100)
  //***********************************************************V1____V2_____V3__vAVG*******************************
  result_v = node_v.readHoldingRegisters(0x63, 8);

  if (result_v == node_v.ku8MBSuccess)
  {
    Serial.println("STARTED READINGS");

    num = node_v.getResponseBuffer(0x01);
    num = num << 16;
    num = num | node_v.getResponseBuffer(0x00);
    f = *((float *)&num);
    float v1 = f;
    Serial.println(v1);
    readings[0] = v1;

    num = node_v.getResponseBuffer(0x03);
    num = num << 16;
    num = num | node_v.getResponseBuffer(0x02);
    f = *((float *)&num);
    float v2 = f;
    Serial.println(v2);
    readings[1] = v2;

    num = node_v.getResponseBuffer(0x05);
    num = num << 16;
    num = num | node_v.getResponseBuffer(0x04);
    f = *((float *)&num);
    float v3 = f;
    Serial.println(v3);
    readings[2] = v3;

    num = node_v.getResponseBuffer(0x07);
    num = num << 16;
    num = num | node_v.getResponseBuffer(0x06);
    f = *((float *)&num);
    float vAvg = f;
    Serial.println(vAvg);
    readings[3] = vAvg;

  }
  //***********************************************************I1____I2_____I3_AVG ******************************* 

  result_i = node_i.readHoldingRegisters(0x79, 8);
  if (result_i == node_i.ku8MBSuccess)
  {
    // delay(20);
    num = node_i.getResponseBuffer(0x01);
    num = num << 16;
    num = num | node_i.getResponseBuffer(0x00);
    f = *((float *)&num);
    float i1 = f;
    readings[4] = i1;

    num = node_i.getResponseBuffer(0x03);
    num = num << 16;
    num = num | node_i.getResponseBuffer(0x02);
    f = *((float *)&num);
    float i2 = f;
    readings[5] = i2;

    num = node_i.getResponseBuffer(0x05);
    num = num << 16;
    num = num | node_i.getResponseBuffer(0x04);
    f = *((float *)&num);
    float i3 = f;
    readings[6] = i3;

    num = node_i.getResponseBuffer(0x07);
    num = num << 16;
    num = num | node_i.getResponseBuffer(0x06);
    f = *((float *)&num);
    float iAvg = f;
    readings[7] = iAvg;

  }
  //***********************************************************PF1____PF2_____PF3*******************************

  result_pf = node_pf.readHoldingRegisters(0x85, 8);
  if (result_pf == node_pf.ku8MBSuccess)
  {
    // delay(20);
    num = node_pf.getResponseBuffer(0x01);
    num = num << 16;
    num = num | node_pf.getResponseBuffer(0x00);
    f = *((float *)&num);
    float pf1 = f;
    readings[8] = pf1;

    num = node_pf.getResponseBuffer(0x03);
    num = num << 16;
    num = num | node_pf.getResponseBuffer(0x02);
    f = *((float *)&num);
    float pf2 = f;
    readings[9] = pf2;

    num = node_pf.getResponseBuffer(0x05);
    num = num << 16;
    num = num | node_pf.getResponseBuffer(0x04);
    f = *((float *)&num);
    float pf3 = f;
    readings[10] = pf3;

    num = node_pf.getResponseBuffer(0x07);
    num = num << 16;
    num = num | node_pf.getResponseBuffer(0x06);
    f = *((float *)&num);
    float pfAvg = f;
    readings[11] = pfAvg;

  }
  //***************************************** KW=phase acvtive power*****************
  result_kw = node_kw.readHoldingRegisters(0x8d, 8);
  if (result_kw == node_kw.ku8MBSuccess)
  {
    // delay(20);
    num = node_kw.getResponseBuffer(0x01);
    num = num << 16;
    num = num | node_kw.getResponseBuffer(0x00);
    f = *((float *)&num);
    float kw1 = f;
    readings[12] = kw1;

    num = node_kw.getResponseBuffer(0x03);
    num = num << 16;
    num = num | node_kw.getResponseBuffer(0x02);
    f = *((float *)&num);
    float kw2 = f;
    readings[13] = kw2;

    num = node_kw.getResponseBuffer(0x05);
    num = num << 16;
    num = num | node_kw.getResponseBuffer(0x04);
    f = *((float *)&num);
    float kw3 = f;
    readings[14] = kw3;

    num = node_kw.getResponseBuffer(0x07);
    num = num << 16;
    num = num | node_kw.getResponseBuffer(0x06);
    f = *((float *)&num);
    float KW = f;
    Serial.println(KW);
    readings[15] = KW;

  }
  //***************************************** KVA =phase Apparent power*****************

  result_kva = node_kva.readHoldingRegisters(0x9d, 8);
  if (result_kva == node_kva.ku8MBSuccess)
  {
    // delay(20);
    num = node_kva.getResponseBuffer(0x01);
    num = num << 16;
    num = num | node_kva.getResponseBuffer(0x00);
    f = *((float *)&num);
    float kva1 = f;
    Serial.println(kva1);
    readings[16] = kva1;

    num = node_kva.getResponseBuffer(0x03);
    num = num << 16;
    num = num | node_kva.getResponseBuffer(0x02);
    f = *((float *)&num);
    float kva2 = f;
    Serial.println(kva2);
    readings[17] = kva2;

    num = node_kva.getResponseBuffer(0x05);
    num = num << 16;
    num = num | node_kva.getResponseBuffer(0x04);
    f = *((float *)&num);
    float kva3 = f;
    Serial.println(kva3);
    readings[18] = kva3;

    num = node_kva.getResponseBuffer(0x07);
    num = num << 16;
    num = num | node_kva.getResponseBuffer(0x06);
    f = *((float *)&num);
    float kva = f;
    Serial.println(kva);
    readings[19] = kva;

  }
}