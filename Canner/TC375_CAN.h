


#ifndef TC375_CAN_H
#define TC375_CAN_H

#include "Arduino.h"
#include "MultiCAN.h"


void CAN_Init_TC375(uint32 Baudrate);

void Can_Rx_Isr(int param_1);
void ProcessFifo0InterruptCan(void);

void CAN_TxInit_TC375(uint32 CAN_Id, uint32 AcceptanceMask, uint8 dlc, uint32 ExtFrame, uint8 MsgObj_Id);
void CAN_RxInit_TC375(uint32 CAN_Id, uint32 AcceptanceMask, uint8 dlc, uint32 ExtFrame, uint8 MsgObj_Id);
int CAN_SendMessage_TC375(uint32 CAN_Id, CANMessagePayloadType *msg1, uint8 dlc, uint8 MsgObj_Id);
IfxCan_Status CAN_ReceiveMessage_TC375(uint32 CAN_Id, CANMessagePayloadType *msg1, uint8 dlc);


#endif //TC375_CAN_H