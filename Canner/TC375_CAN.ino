
#include <Arduino.h>
#include "TC375_CAN.h"
#include "MultiCAN.h"

/* TODO add checks in TX/RX funcs msg_objId is not too big
*  Make work on all cores currently only works for core 0
*  Fill level, let recv know how many bytes it got, to know  if zeros recived is ment for or if nothing
*  It works test but ghidra showed some other things Which i have not added IDK
*/

App_Can g_App_Can;

void CAN_Init_TC375(uint32 Baudrate)
{
  //contain values about core
  //byte in_CORE_ID;
  //uint in_ICR;

  IfxCan_Can_NodeConfig nodeConfig;
  IfxCan_Can_Pins canPins;
  IfxCan_Can_Config canConfig[7];
  IfxCan_NodeId nodeId;

  IfxCan_Can_initModuleConfig(canConfig, &MODULE_CAN1);
  IfxCan_Can_initModule(&g_App_Can.can[0], canConfig);

  /* CHECK with in_ICR occurs here */

  if (g_App_Can.CAN_Initialised == CANStructuresNotInitialised)
  {
    g_App_Can.CAN_Initialised = CANStructuresInitialised;
    for(int i=0; i<REQUIREDNUMBEROFRXFILTERS; i++) { g_App_Can.UserCanRxIds[i] = 0xffffffff; }
    for(int i=0; i<REQUIREDNUMBEROFTXOBJECTS; i++) { g_App_Can.UserCanTxIds[i] = 0xffffffff; }
  }

  IfxCan_Can_initNodeConfig(&nodeConfig, &g_App_Can.can[0]);

  nodeConfig.nodeId                                          = IfxCan_NodeId_0;
  nodeConfig.clockSource                                     = IfxCan_ClockSource_both;
  nodeConfig.frame.type                                      = IfxCan_FrameType_transmitAndReceive;
  nodeConfig.txConfig.dedicatedTxBuffersNumber               = 1;
  nodeConfig.messageRAM.baseAddress                          = (uint32) &MODULE_CAN1;
  nodeConfig.messageRAM.txBuffersStartAddress                = 0x100;
  nodeConfig.messageRAM.rxBuffersStartAddress                = 0x600;
  nodeConfig.messageRAM.rxFifo1StartAddress                  = 0xf00;
  nodeConfig.messageRAM.rxFifo0StartAddress                  = 0x1800;
  nodeConfig.messageRAM.standardFilterListStartAddress       = 0x2100;
  nodeConfig.rxConfig.rxMode                                 = IfxCan_RxMode_fifo0;
  nodeConfig.rxConfig.rxFifo0Size                            = 32;
  nodeConfig.rxConfig.rxFifo1Size                            = 32;
  nodeConfig.rxConfig.rxFifo0OperatingMode                   = IfxCan_RxFifoMode_overwrite;
  nodeConfig.rxConfig.rxFifo1OperatingMode                   = IfxCan_RxFifoMode_blocking;
  nodeConfig.filterConfig.standardListSize                   = 32;
  nodeConfig.filterConfig.standardFilterForNonMatchingFrames = IfxCan_NonMatchingFrame_acceptToRxFifo1;

  canPins.rxPin     = &IfxCan_RXD10C_P23_0_IN;
  canPins.rxPinMode = IfxPort_InputMode_pullUp;
  canPins.txPin     = &IfxCan_TXD10_P13_0_OUT;
  canPins.txPinMode = IfxPort_OutputMode_pushPull;
  canPins.padDriver = IfxPort_PadDriver_cmosAutomotiveSpeed2;
  nodeConfig.pins   = &canPins;

  nodeConfig.interruptConfig.rxFifo0NewMessageEnabled = 1;
  nodeConfig.interruptConfig.rxf0n.priority           = 200;
  nodeConfig.interruptConfig.rxf0n.interruptLine      = IfxCan_InterruptLine_14;
  nodeConfig.interruptConfig.rxf0n.typeOfService      = IfxSrc_Tos_cpu0; //set interrupt on CORE 0 //normally = (in_CORE_ID & 7)

  nodeConfig.baudRate.baudrate = Baudrate;

  IfxCan_Can_initNode(&g_App_Can.canNode[0], &nodeConfig);

  InterruptInit();
  InterruptInstall(0x18a, Can_Rx_Isr, 200, 0);
  

  /*Some bitmasking happens here idk*/

  for (int i=0; i<REQUIREDNUMBEROFRXFILTERS; i++)
  {
    g_App_Can.RxFilters[i].number               = 0;
    g_App_Can.RxFilters[i].elementConfiguration = IfxCan_FilterElementConfiguration_storeInRxFifo0;
    g_App_Can.RxFilters[i].type                 = IfxCan_FilterType_classic;
    g_App_Can.RxFilters[i].id1                  = 0xffffffff;
    g_App_Can.RxFilters[i].id2                  = 0x7fffffff;
    g_App_Can.RxFilters[i].rxBufferOffset       = IfxCan_RxBufferId_0;
    g_App_Can.RxFilters[i].number               = i;
    g_App_Can.CanRxMessageObject[i].NewData     = NoNewData;
  }

}


void Can_Rx_Isr(int param_1)
{
  SerialASC.println("CAN_Rx_Isr");
  g_App_Can.InterruptProcessed = 1;
  IfxCan_Node_clearInterruptFlag(g_App_Can.canNode[0].node, IfxCan_Interrupt_rxFifo0NewMessage);
  ProcessFifo0InterruptCan();
}

void ProcessFifo0InterruptCan(void)
{
  CANMessagePayloadType msgData;
  IfxCan_Message CanMessage;
  IfxCan_Can_initMessage(&CanMessage);
  // bit masking CanMessage._16_4_ <- need to figure out
  CanMessage.readFromRxFifo0 = 1;

  IfxCan_Can_readMessage(&g_App_Can.canNode[0], &CanMessage, msgData.dword); //might be weird here with canNode
  for (int i=0; i<REQUIREDNUMBEROFRXFILTERS; i++)
  {
    if(CanMessage.messageId == g_App_Can.RxFilters[i].id1)
    {
      g_App_Can.CanRxMessageObject[i].ReceivedMessage.bufferNumber               = CanMessage.bufferNumber;
      g_App_Can.CanRxMessageObject[i].ReceivedMessage.messageId                  = CanMessage.messageId;
      g_App_Can.CanRxMessageObject[i].ReceivedMessage.remoteTransmitRequest      = CanMessage.remoteTransmitRequest;
      g_App_Can.CanRxMessageObject[i].ReceivedMessage.messageIdLength            = CanMessage.messageIdLength;
      g_App_Can.CanRxMessageObject[i].ReceivedMessage.errorStateIndicator        = CanMessage.errorStateIndicator;
      g_App_Can.CanRxMessageObject[i].ReceivedMessage.dataLengthCode             = CanMessage.dataLengthCode;
      g_App_Can.CanRxMessageObject[i].ReceivedMessage.frameMode                  = CanMessage.frameMode;
      g_App_Can.CanRxMessageObject[i].ReceivedMessage.txEventFifoControl         = CanMessage.txEventFifoControl;
      g_App_Can.CanRxMessageObject[i].ReceivedMessage.storeInTxFifoQueue         = CanMessage.storeInTxFifoQueue;
      g_App_Can.CanRxMessageObject[i].ReceivedMessage.readFromRxFifo0            = CanMessage.readFromRxFifo0;
      g_App_Can.CanRxMessageObject[i].ReceivedMessage.readFromRxFifo1            = CanMessage.readFromRxFifo1;
  
      g_App_Can.CanRxMessageObject[i].NewData   = NewDataReceived;
      g_App_Can.CanRxMessageObject[i].Payload   = msgData;
      //g_App_Can.CanRxMessageObject[i].FillLevel = 8; not sure about this will get to later might be usefull to know how many bytes recieved
    }
  }
}



void CAN_TxInit_TC375(uint32 CAN_Id, uint32 AcceptanceMask, uint8 dlc, uint32 ExtFrame, uint8 MsgObj_Id)
{
/* NOTES:
    Somehow they mange to get a variable with array postion, taking array_num = 0 
    That value is used to check if it is not full,
    Used to shift the base address
    *** ADD CHECK SO MsgObj_Id is not too big ***
*/
  IfxCan_Message CanTxMessage;

  g_App_Can.UserCanTxIds[MsgObj_Id] = CAN_Id;
  IfxCan_Can_initMessage(&CanTxMessage);

  g_App_Can.UserCanTxMessages[MsgObj_Id].bufferNumber          = 0;
  g_App_Can.UserCanTxMessages[MsgObj_Id].messageId             = CAN_Id;
  g_App_Can.UserCanTxMessages[MsgObj_Id].remoteTransmitRequest = CanTxMessage.remoteTransmitRequest;
  g_App_Can.UserCanTxMessages[MsgObj_Id].messageIdLength       = (ExtFrame == 29) ? IfxCan_MessageIdLength_extended : IfxCan_MessageIdLength_standard;
  g_App_Can.UserCanTxMessages[MsgObj_Id].errorStateIndicator   = CanTxMessage.errorStateIndicator;
  g_App_Can.UserCanTxMessages[MsgObj_Id].dataLengthCode        = (IfxCan_DataLengthCode) dlc;
  g_App_Can.UserCanTxMessages[MsgObj_Id].frameMode             = IfxCan_FrameMode_standard;
  g_App_Can.UserCanTxMessages[MsgObj_Id].txEventFifoControl    = CanTxMessage.txEventFifoControl;
  g_App_Can.UserCanTxMessages[MsgObj_Id].storeInTxFifoQueue    = CanTxMessage.storeInTxFifoQueue;
  g_App_Can.UserCanTxMessages[MsgObj_Id].readFromRxFifo0       = CanTxMessage.readFromRxFifo0;
  g_App_Can.UserCanTxMessages[MsgObj_Id].readFromRxFifo1       = CanTxMessage.readFromRxFifo1;
}

void CAN_RxInit_TC375(uint32 CAN_Id, uint32 AcceptanceMask, uint8 dlc, uint32 ExtFrame, uint8 MsgObj_Id)
{
  /* NOTES:
    Somehow they mange to get a variable with array postion, taking array_num = 0 
    That value is used to check if it is not full,
    Used to shift the base address
    Assuming this is MsgObj_Id
    *** ADD CHECK SO MsgObj_Id is not too big ***
*/
  IfxCan_Can_NodeConfig nodeConfig;

  g_App_Can.UserCanRxIds[MsgObj_Id] = CAN_Id;
  IfxCan_Can_initNodeConfig(&nodeConfig, g_App_Can.can);

  g_App_Can.RxFilters[MsgObj_Id].number               = MsgObj_Id;
  g_App_Can.RxFilters[MsgObj_Id].elementConfiguration = IfxCan_FilterElementConfiguration_storeInRxFifo0;
  g_App_Can.RxFilters[MsgObj_Id].type                 = IfxCan_FilterType_classic;
  g_App_Can.RxFilters[MsgObj_Id].id1                  = CAN_Id;
  g_App_Can.RxFilters[MsgObj_Id].id2                  = AcceptanceMask;
  g_App_Can.RxFilters[MsgObj_Id].rxBufferOffset       = IfxCan_RxBufferId_0;

  //set extended or standard
  if (ExtFrame == 29) //extended
  {
    IfxCan_Can_setExtendedFilter(&g_App_Can.canNode[0], &g_App_Can.RxFilters[MsgObj_Id]);
  }else //standard
  { 
    IfxCan_Can_setStandardFilter(&g_App_Can.canNode[0], &g_App_Can.RxFilters[MsgObj_Id]);
  }
}

int CAN_SendMessage_TC375(uint32 CAN_Id, CANMessagePayloadType *msg1, uint8 dlc, uint8 MsgObj_Id)
{
  //assuming zero for now so no need  for it FindCanTxID_Offset(CAN_Id); = 0
  IfxCan_Message CanTxMessage;
  int timeout = 0;
  IfxCan_Status TxStatus;
  bool bVar1;
  //In Orginal Offset (which array number) is got by getting the offset of the CAN ID, I.e which TxMessage has that ID,
  //I will just reuse MsgObj_Id because I am lazy but make sure the Init and Send funcs have same MsgObj_Id
  //*** ADD CHECK SO MsgObj_Id is not too big ***

  CanTxMessage.bufferNumber          = g_App_Can.UserCanTxMessages[MsgObj_Id].bufferNumber;
  CanTxMessage.messageId             = CAN_Id;
  CanTxMessage.remoteTransmitRequest = g_App_Can.UserCanTxMessages[MsgObj_Id].remoteTransmitRequest;
  CanTxMessage.messageIdLength       = g_App_Can.UserCanTxMessages[MsgObj_Id].messageIdLength; 
  CanTxMessage.errorStateIndicator   = g_App_Can.UserCanTxMessages[MsgObj_Id].errorStateIndicator;
  CanTxMessage.dataLengthCode        = (IfxCan_DataLengthCode) dlc; //need to figure out how this relates to  dlc input
  CanTxMessage.frameMode             = g_App_Can.UserCanTxMessages[MsgObj_Id].frameMode;
  CanTxMessage.txEventFifoControl    = g_App_Can.UserCanTxMessages[MsgObj_Id].txEventFifoControl;
  CanTxMessage.storeInTxFifoQueue    = g_App_Can.UserCanTxMessages[MsgObj_Id].storeInTxFifoQueue;
  CanTxMessage.readFromRxFifo0       = g_App_Can.UserCanTxMessages[MsgObj_Id].readFromRxFifo0;
  CanTxMessage.readFromRxFifo1       = g_App_Can.UserCanTxMessages[MsgObj_Id].readFromRxFifo1;

  while(true){
   TxStatus = IfxCan_Can_sendMessage(&g_App_Can.canNode[0], &CanTxMessage, msg1->dword);
   if ((TxStatus == IfxCan_Status_notSentBusy ) && (timeout < 50000)){
    bVar1 = true;
   }
   else {
    bVar1 = false;
   }
   if (!bVar1) break;
   timeout++;
  }
  return timeout != 50000;
}

IfxCan_Status CAN_ReceiveMessage_TC375(uint32 CAN_Id, CANMessagePayloadType *msg1, uint8 dlc, uint8 MsgObj_Id)
{
  //In Orginal Offset (which array number) is got by getting the offset of the CAN ID, I.e which TxMessage has that ID,
  //I will just reuse MsgObj_Id because I am lazy but make sure the Init and Send funcs have same MsgObj_Id
  //*** ADD CHECK SO MsgObj_Id is not too big ***

  IfxCan_Message CanRxMessage;
  IfxCan_Status RxStatus = IfxCan_Status_ok;
  int timeout = 0;                                                   

  IfxCan_Can_initMessage(&CanRxMessage);

  for(; g_App_Can.CanRxMessageObject[MsgObj_Id].NewData == NoNewData &&  (timeout < 2000); timeout++);
  if (timeout == 2000)
  {
    g_App_Can.CanRxMessageObject[MsgObj_Id].NewData = NoNewData;
    RxStatus = IfxCan_Status_receiveEmpty;
  }else
  {
    *msg1 = g_App_Can.CanRxMessageObject[MsgObj_Id].Payload;
    g_App_Can.CanRxMessageObject[MsgObj_Id].NewData = NoNewData;
    g_App_Can.InterruptProcessed = 0;
  }
  return RxStatus;
}