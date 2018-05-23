#include "sdDriver.h"
#include <stdio.h>
#include <memory.h>
#include <config.h>
#include <util/debug.h>
#include <hardware/hardware.h>

static uint32_t cardBlockCount =0;

//send standard 6 byte command & look for response
static uint8_t sendCommand(uint8_t * cmd){
  uint8_t res = 0x0;
  uint8_t noCmd = 0xFF;

  //send nop before command
  sendSPI(&noCmd, NULL, 1);// <-- REALLY FUCKING IMPORTANT

  //send command
  sendSPI(cmd, NULL, 6);

  // wait for response
  for(int z = 0; z < TIMEOUT_CLK; z++){
    res = 0x0;
    sendSPI(&noCmd, &res, 1);
    if((res >> 7) == 0x00){
      return res;
    }
  }

  return 0xFF;
}

//get next byte in SPI "stream"
static uint8_t getNextByte(){
  uint8_t fake = 0xFF;
  uint8_t res = 0x0;
  sendSPI(&fake, &res, 1);
  return res;
}

static void writeNextByte(uint8_t byte){
  sendSPI(&byte,NULL,1);
}

/** wait for SD to become ready **/
static void waitForCard(){
  while(getNextByte() != 0xFF){
    sleep(1);
  }
}

static bool checkAddress(uint32_t addr){
  if(addr <= cardBlockCount){
    return true;
  }
  return false;
}

//send a bunch of nops down the SPI channel
static void flushChannel(){
  uint8_t fake = 0xFF;
  for(int i =0; i < 512; i ++){
    sendSPI(&fake,NULL,1);
  }
}

bool initDefault(void){
  /*
    what this function does:
    1. start SPI on MCU
    2. clock at least 80 SPI clock cycles without selecting the sd card (CS high)
    3. send CMD0 to switch the card to SPI mode
    4. send CMD8 to query SD voltage range (but, ignore result :P) <- [looks optional but, is required]
    5. send ACMD41 to switch the card from idle to active mode
  */

  initSPI(toPin(PIO_BANK_A,PIO_PA6,NULL));
  int tOut = 0;

  //send 80 "fake" bits
  uint8_t fake = 0xFF;
  setCS(1);
  for(int i =0; i < 80; i++){
    sendSPI(&fake,NULL,1);
  }

  //send CMD0
  setCS(0);
  uint8_t cmd0[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
  tOut = 0;
  for(;;){
    uint8_t r1 = sendCommand(cmd0);
    if(r1 == 0x01){
      break;
    }

    sleep(100);
    if(tOut++ > TIMEOUT_CLK){
      printf("SD Card init error CMD0 Timeout\n");
      return false;
    }
  }

  //send CMD8
  uint8_t cmd8[] = {0x48, 0x00, 0x00, 0x01, 0xAA, 0x87};
  tOut = 0;
  while(sendCommand(cmd8) != 0x01){
    sleep(100);
    if(tOut++ > TIMEOUT_CLK){
      printf("SD Card init error CMD8 Timeout\n");
      return false;
    }
  }
  uint8_t res[4];
  res[0] = getNextByte();
  res[1] = getNextByte();
  res[2] = getNextByte();
  res[3] = getNextByte();

  if(res[0] != cmd8[1] || res[1] != cmd8[2] || res[2] != cmd8[3] || res[3] != cmd8[4]){
    printf("SD Card init error, CMD8 echo incorrect\n");
    return false;
  }

  //send ACMD41 to initialize card
  //it is not unusual to have to send this command sequence multiple times
  uint8_t cmd55[] = {0x77, 0x00, 0x00, 0x00, 0x00, 0x65};
  uint8_t cmd41[] = {0x69, 0x40, 0x00, 0x00, 0x00, 0x77};
  for(tOut = 0; tOut < TIMEOUT_CLK; tOut++){
    if(sendCommand(cmd55) == 0x01){
      if(sendCommand(cmd41) == 0x00){
        break;
      }
    }
    sleep(100);
    flushChannel();
    sleep(100);
  }

  if(tOut == TIMEOUT_CLK){
    printf("SD Card init error ACMD41 Timeout\n");
    return false;
  }

  //get SD size
  uint8_t csd[16];
  memset(csd, 0, 16);
  if(!getCardCSD(csd)){
    printf("SD Card init error, cannot get CSD register\n");
    return false;
  }
  cardBlockCount = ((((uint32_t)csd[7] & 0x3F) << 16) | ((uint16_t)csd[8] << 8) | csd[9]) + 1;

  return true;
}

bool getCardCSD(uint8_t * buffer){
  uint8_t cmd9[] = {0x49, 0x00, 0x00, 0x00, 0x00, 0x01};

  if(sendCommand(cmd9) == 0x00){
    int z =0;
    for(z=0; z < TIMEOUT_RW_BLOCK; z++){
      if(getNextByte() == 0xFE){
        break;
      }
    }

    if(z != TIMEOUT_RW_BLOCK){
      for(int i =0; i < CSD_RESPONSE_SIZE; i++){
        buffer[i] = getNextByte();
      }
      waitForCard();
      return true;
    }
    else {
      waitForCard();
      return false;
    }
  }
  else {
    waitForCard();
    return false;
  }
}

uint32_t getCardSize(){
  return cardBlockCount;
}

uint16_t getBlockSize(){
  return RW_BLOCK_SIZE;
}

bool readBlock(uint8_t * buffer, uint32_t addr){
  if(!checkAddress(addr)){
    return false;
  }

  uint8_t cmd17[6];
  cmd17[0] = 0x51;
  cmd17[1] = 0xFF & (addr >> 24);
  cmd17[2] = 0xFF & (addr >> 16);
  cmd17[3] = 0xFF & (addr >> 8 );
  cmd17[4] = 0xFF & (addr      );
  cmd17[5] = 0x01;

  if(sendCommand(cmd17) == 0x00){
    int z =0;
    for(z=0; z < TIMEOUT_RW_BLOCK; z++){
      if(getNextByte() == 0xFE){
        break;
      }
    }

    if(z != TIMEOUT_RW_BLOCK){
      //read block
      for(int i = 0; i < RW_BLOCK_SIZE; i++){
        buffer[i] = getNextByte();
      }

      //CRC is for chumps discard
      getNextByte();
      getNextByte();

      waitForCard();
      return true;
    }
    else{
      waitForCard();
      return false;
    }
  }
  else {
    waitForCard();
    return false;
  }
}


bool writeBlock(uint8_t * buffer, uint32_t addr){
  if(!checkAddress(addr)){
    return false;
  }

  uint8_t cmd24[6];
  cmd24[0] = 0x58;
  cmd24[1] = 0xFF & (addr >> 24);
  cmd24[2] = 0xFF & (addr >> 16);
  cmd24[3] = 0xFF & (addr >> 8 );
  cmd24[4] = 0xFF & (addr      );
  cmd24[5] = 0x01;

  if(sendCommand(cmd24) == 0x00){
    getNextByte();// delay one byte

    writeNextByte(0xFE); // data block header
    for(int i =0; i < RW_BLOCK_SIZE; i++){
      writeNextByte(buffer[i]);
    }

    //CRC is for chumps
    getNextByte();
    getNextByte();

    //wait for response
    for(int z = 0; z < TIMEOUT_CLK; z ++){
      uint8_t r = getNextByte();
      if((0x01 & (r >> 4)) == 0x00){
        if((0x0F & r) == 0x5){
          //block written!
          waitForCard();
          return true;
        }
        else {
          break;
        }
      }
    }
    //fail for some reason
    waitForCard();
    return false;
  }
  else{
    waitForCard();
    return false;
  }
}
