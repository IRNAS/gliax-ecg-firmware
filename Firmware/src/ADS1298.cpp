/*
 * This file is part of MobilECG, an open source clinical grade Holter
 * ECG. For more information visit http://mobilecg.hu
 *
 * Copyright (C) 2016  Robert Csordas, Peter Isza
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "ADS1298.h"
#include "Logger.h"
#include <string.h>

#define HR 0x80

#define C2_INTERNAL_TEST_SINGAL_ON (1<<4)

#define C3_FIXED 0x40
#define C3_INTERNAL_REFERENCE_POWER_ON 0x80
#define C3_INTERNAL_RLD_REFERENCE (1<<3)
#define C3_RLD_POWER_ON (1<<2)
#define C3_RLD_SENSE_ON (1<<1)
#define C3_RLD_LEAD_OFF_STATUS (1<<0)

#define WCT1_WCTA_POWER_ON (1<<3)

#define WCT1_WCTA_CHANNEL1_POS 0
#define WCT1_WCTA_CHANNEL1_NEG 1
#define WCT1_WCTA_CHANNEL2_POS 2
#define WCT1_WCTA_CHANNEL2_NEG 3
#define WCT1_WCTA_CHANNEL3_POS 4
#define WCT1_WCTA_CHANNEL3_NEG 5
#define WCT1_WCTA_CHANNEL4_POS 6
#define WCT1_WCTA_CHANNEL4_NEG 7

#define WCT2_WCTC_POWER_ON (1<<7)
#define WCT2_WCTB_POWER_ON (1<<6)

#define WCT2_WCTB_CHANNEL1_POS (0<<3)
#define WCT2_WCTB_CHANNEL1_NEG (1<<3)
#define WCT2_WCTB_CHANNEL2_POS (2<<3)
#define WCT2_WCTB_CHANNEL2_NEG (3<<3)
#define WCT2_WCTB_CHANNEL3_POS (4<<3)
#define WCT2_WCTB_CHANNEL3_NEG (5<<3)
#define WCT2_WCTB_CHANNEL4_POS (6<<3)
#define WCT2_WCTB_CHANNEL4_NEG (7<<3)

#define WCT2_WCTC_CHANNEL1_POS 0
#define WCT2_WCTC_CHANNEL1_NEG 1
#define WCT2_WCTC_CHANNEL2_POS 2
#define WCT2_WCTC_CHANNEL2_NEG 3
#define WCT2_WCTC_CHANNEL3_POS 4
#define WCT2_WCTC_CHANNEL3_NEG 5
#define WCT2_WCTC_CHANNEL4_POS 6
#define WCT2_WCTC_CHANNEL4_NEG 7

#define CH_GAIN_OFFSET 4

//#define DEBUG
//#define TEST_SIGNAL

const float ADS1298::ECG_LSB_IN_MV = 0.0001430511475f;

SPI spi(PB_15, PB_14, PB_13);	// mosi, miso, sclk
DigitalOut cs(PB_12);	// spi cs

InterruptIn drdy(PA_4); // interrupt for data available pin

void InterruptHandler() {		// new interrupt function
	ADS1298::instance().interrupt();
}

ADS1298::ADS1298():
        reset(PA_6, 1),
        pwdn(PA_7),
        pinStart(PB_10,0),
        diffSel(PC_4)
{
	diffSel = 0;
	memset(zeroBuffer, 0, ADS1298_MAX_PACKET_LENGTH);
	dmaRunning=false;
	//Default PGA is 6
	currLsbInMv = ECG_LSB_IN_MV / 6.0f;

    //Init SPI
    cs = 1;	// chip is deselected
    spi.format(8, 1);	// num. ob bits, mode(means polarity=0 in phase=1)
    spi.frequency(1000000);	// SPI frequency 1 MHz
}

ADS1298& ADS1298::instance(){
	static ADS1298 inst;
	return inst;
}

void ADS1298::writeReg(Register reg, uint8_t value){
	uint8_t writeBuf[3];
	writeBuf[0]=CMD_WREG | (uint8_t)reg;
	writeBuf[1]=0;
	writeBuf[2]=value;

	uint8_t readBuf[3];
	cs = 0;
	spi.write((const char *)writeBuf, sizeof(writeBuf), (char *)readBuf, sizeof(readBuf));
	cs = 1;
}

uint8_t ADS1298::readReg(Register reg){
	uint8_t writeBuf[3];
	writeBuf[0]=CMD_RREG | (uint8_t)reg;
	writeBuf[1]=0;
	writeBuf[2]=0;

	uint8_t readBuf[3];
	cs = 0;
	spi.write((const char *)writeBuf, sizeof(writeBuf), (char *)readBuf, sizeof(readBuf));
	cs = 1;
 	return readBuf[2];
}

void ADS1298::sendCommand(Command cmd){
	char data=cmd;
	char dummy;
	cs = 0;
	spi.write((const char *)(&data), sizeof(data), &dummy, sizeof(dummy));
	cs = 1;
}

bool ADS1298::start(Serial *pc){
	pwdn = 1;	// power on chip
	#ifdef DEBUG
		pc->printf("Point 1\n");
	#endif
	osDelay(100);
	
	reset = 0;	// reset chip
	osDelay(20);
	reset = 1;
	#ifdef DEBUG
		pc->printf("Point 2\n");
	#endif
	
	osDelay(20);	// send command
	sendCommand(CMD_SDATAC);
	osDelay(20);
	#ifdef DEBUG
		pc->printf("Point 3\n");
	#endif
	
	uint8_t id = readReg(REG_ID);
	pc->printf("device ID: %d\n", id);
	if ((id >> 3) != 0x12 && (id >> 3)!= 0x1A){
		//Wrong device signature
		stop();
		pc->printf("Wrong device signature\n");
		return false;
	}

	nHardwareChannels = id & 0x07;
	if (nHardwareChannels > 2){
		//Wrong channel number
		stop();
		pc->printf("Wrong channel number\n");
		return false;
	}

	nHardwareChannels = 4+2*nHardwareChannels;

	selectedChannels = nHardwareChannels;
	dataTransferSize = (selectedChannels + 1)*3;
	int bufSize=ecgBuffer.maxSize();
	bufSize -= bufSize % dataTransferSize;
	ecgBuffer.resize(bufSize-1);
	#ifdef DEBUG
		pc->printf("Selected channels: %d\n", selectedChannels);
	#endif

	uint8_t rldChannels = (1 << 1) | (1 << 2);
	writeReg(REG_RLD_SENSN, rldChannels);
	writeReg(REG_RLD_SENSP, rldChannels);

	// Enable internal reference
	writeReg(REG_CONFIG3,
			C3_FIXED |
			C3_INTERNAL_REFERENCE_POWER_ON |
			C3_INTERNAL_RLD_REFERENCE |
			C3_RLD_POWER_ON);


	// Turn on WCT amplifier
	writeReg(REG_WCT1,
			WCT1_WCTA_POWER_ON |
			WCT1_WCTA_CHANNEL2_NEG);

	writeReg(REG_WCT2,
			WCT2_WCTB_POWER_ON |
			WCT2_WCTB_CHANNEL2_POS |
			WCT2_WCTC_POWER_ON |
			WCT2_WCTC_CHANNEL3_POS);


	//Set gain
	setGain(2);

	setSpeed(DIV_4096);

	//Magic
	uint8_t conf2 = 0;
	#ifdef TEST_SIGNAL
		conf2 += C2_INTERNAL_TEST_SINGAL_ON;
		conf2 += 1;		// change frequency to 2 Hz (otherwise is 1 Hz)
	#endif
	writeReg(REG_CONFIG2, conf2);
	
	sendCommand(CMD_START);
	sendCommand(CMD_RDATAC);

	enableIrq();
	
	return true;
}

float ADS1298::setSpeed(SpeedDiv div, bool highRes){
	float fmod=((float)FCLK) / (highRes ? 4 : 8);

	if (((!highRes) && div==0) || (highRes && div>=DIV_8192))
		Logger::panic("ADS1298::setSpeed(): Invalid params");

	if (!highRes)
		div = (SpeedDiv)(div-1);

	writeReg(REG_CONFIG1, ((uint8_t)div) | (highRes ? HR : 0));

	realFreq = fmod / (1<<(4+div));

	return realFreq;
}

void ADS1298::interrupt(){
	uint8_t *buffer;

	sampleId++;

	if (dmaRunning) {
		ecgBuffer.added(dataTransferSize);
	}

	if (ecgBuffer.getContinuousWriteBuffer(buffer) < dataTransferSize){
		dmaRunning=false;
		return;
	}

	dmaRunning=true;
	cs = 0;
	spi.write((const char *)zeroBuffer, dataTransferSize, (char *)buffer, dataTransferSize);
	cs = 1;
}

void ADS1298::stop(){
	disableIrq();
	dmaRunning=false;
	sendCommand(CMD_SDATAC);
	osDelay(20);
	sendCommand(CMD_STOP);
}

int ADS1298::getAvailableData(){
	uint32_t size = ecgBuffer.used();
	uint32_t blockSize = (getActiveChannels()+1)*3;
	return size - size % blockSize;
}

void ADS1298::getSample(int32_t *data){
	uint8_t *src;
	uint8_t *blockStart;
	uint8_t *dest=(uint8_t*)data;

	ecgBuffer.getContinuousReadBuffer(blockStart);

	//Skip the status.
	src=blockStart+3;

	//Read, fix endianness, and sign extend
	for (int c=0; c<selectedChannels; c++, src+=3){
		*(dest++)=src[2];
		*(dest++)=src[1];
		*(dest++)=src[0];
		*(dest++)=(src[0] & 0x80) ? 0xFF : 0x00;
	}

	ecgBuffer.skip(src - blockStart);
}

void ADS1298::clear(){
	dmaRunning=false;
	ecgBuffer.clear();
}

uint32_t ADS1298::getSampleId(){
	return sampleId;
}

uint8_t ADS1298::getActiveChannels(){
	return selectedChannels;
}

void ADS1298::disableIrq(){
	drdy.fall(NULL);
}

void ADS1298::enableIrq(){
	drdy.fall(&InterruptHandler);	// drdy goes low when new data is available
}

float ADS1298::getLsbInMv(){
	return currLsbInMv;
}

void ADS1298::setGain(int gain){
	currLsbInMv = ECG_LSB_IN_MV / gain;

	int g=0;
	switch (gain){
		case(1):
			g=1;
			break;
		case(2):
			g=2;
			break;
		case(3):
			g=3;
			break;
		case(4):
			g=4;
			break;
		case(6):
			g=0;
			break;
		case(8):
			g=5;
			break;
		case(12):
			g=6;
			break;
		default:
			Logger::panic("Invalid gain.");
			break;
	}

	for (int a=0; a<8; a++){
		uint8_t value = (g<<CH_GAIN_OFFSET);
	#ifdef TEST_SIGNAL
		value += 5;
	#endif
		writeReg(chSetReg(a), value);
	}
}

ADS1298::Register ADS1298::chSetReg(int channel){
	return (Register)(0x05+channel);
}
