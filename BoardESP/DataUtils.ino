#include "StatementType.h"

const byte TERMINATING_BYTE = 254;

enum BoardStateRequestType {
  FULL_STATE,
  ACTIONABLE_STATE
};


byte toByte(bool b[8])
{
  byte res = 0;
  for (int i = 0; i < 8; ++i)
    if (b[i])
      res |= 1 << i;
  return res;
};

byte* sensorDataToBytes(bool closeToWall, bool red, bool green, bool blue) {
  return {toByte((bool[]){blue, green, red, closeToWall, false, false, false, false}), 255};
};

unsigned char[] statementToBytes(enum StatementType type, byte argument) {
  return {(byte) type, argument, 255};
};
