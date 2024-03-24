#include "TC375_CAN.h"

void setup() {
  //enable Interrupts
  IfxCpu_enableInterrupts();

  //disable watchdog
  IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
  IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

  SerialASC.begin(115200);
 
  CAN_Init_TC375(500000);
  
  CAN_TxInit_TC375(0x250, 0x7FFU, 8, 11u, 0);
  CAN_TxInit_TC375(0x300, 0x7FFU, 8, 11u, 1);

  CAN_RxInit_TC375(0x150, 0x7FFU, 8, 11u, 0);
  CAN_RxInit_TC375(0x125, 0x7FFU, 8, 11u, 1);
}

void loop() {
  CANMessagePayloadType sendMsg1, sendMsg2, recvMsg1, recvMsg2;

  sendMsg1.dword[0] = 0xFAFAFAFA;
  sendMsg1.dword[1] = 0xAFAFAFAF;

  sendMsg2.dword[0] = 0xF0F0F0F0;
  sendMsg2.dword[1] = 0x0F0F0F0F;

  CAN_SendMessage_TC375(0x250, &sendMsg1, 8, 0) ? SerialASC.println("CAN SENT to 0x250") : SerialASC.println("CAN SET FAILED to 0x250");
  delay(100);
  CAN_SendMessage_TC375(0x300, &sendMsg2, 8, 1) ? SerialASC.println("CAN SENT to 0x300") : SerialASC.println("CAN SET FAILED to 0x300");

  delay(500);
  IfxCan_Status RxStatus1 = CAN_ReceiveMessage_TC375(0x150, &recvMsg1, 8, 0);
  switch(RxStatus1) //print status of received message
  {
    case IfxCan_Status_ok:               //0x00000000
      SerialASC.print("0x150 Receive OK : ");
      for(int i=0; i<8; i++){SerialASC.println(recvMsg1.bytes[i], HEX);}
      break;
    case IfxCan_Status_receiveEmpty:      //0x00000040
      SerialASC.println("0x150 Receive empty");
      break;
    default:
      SerialASC.println("Unknown status");
      break;
  }

  delay(500);
  IfxCan_Status RxStatus2 = CAN_ReceiveMessage_TC375(0x150, &recvMsg2, 8, 1);
  switch(RxStatus2) //print status of received message
  {
    case IfxCan_Status_ok:               //0x00000000
      SerialASC.print("0x125 Receive OK : ");
      for(int i=0; i<8; i++){SerialASC.println(recvMsg2.bytes[i], HEX);}
      break;
    case IfxCan_Status_receiveEmpty:      //0x00000040
      SerialASC.println("0x125 Receive empty");
      break;
    default:
      SerialASC.println("Unknown status");
      break;
  }
  
}
