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

#include "mbed.h"
#include "cmsis_os.h"
#include "FastPWM.h"
#include "ADS1298.h"
#include "Logger.h"
#include "Packetizer.h"
#include "ECGSender.h"

//#define DEBUG

DigitalOut led1(PB_6);
DigitalOut led2(PB_7);

DigitalOut en2v8(PC_1);
DigitalOut en3v3(PB_8);

FastPWM clk(PA_10,1);	// clock pin for ADS

Serial pc(PA_2, PA_3);	// UART connection for sending data

Packetizer dataPacketizer;
ECGSender ecgSender(dataPacketizer);

void mainTaskCallback(void const *args) 
{
	osDelay(100);
	en2v8 = 1;
	osDelay(100);

    //Initialize UART here
    pc.baud(115200);
    #ifdef DEBUG
		pc.printf("Test started\n");
	#endif
    
    //Turn on ECG clock	
    clk.period_ticks(50);	// chip frequency = 100 mhz / 50 = 2 mhz (which ads chip needs)
    clk.pulsewidth_ticks(25);
	
	if (!ADS1298::instance().start(&pc))
    {
        #ifdef DEBUG
			pc.printf("ADS1298 not started!!!\n");
		#endif
		Logger::panic("Failed to initialize ADS1298.");
    }
	while(1)
    {
		if (ADS1298::instance().getAvailableData() < 512)
        {
			#ifdef DEBUG
				pc.printf("Not enough data available...\n");
			#endif
			osDelay(10);
			continue;
		}
		
		#ifdef DEBUG
			pc.printf("Sending data to pc...\n");
		#endif
		ecgSender.send(&pc);
	}
}

osThreadDef(mainTaskCallback, osPriorityNormal, 4096);

void ledTaskCallback(void const *args)
{
	while(1)
    {
		led1 = 1;
		led2 = 0;
		osDelay(1000);

		led1 = 0;
		led2 = 1;
		osDelay(1000);
	}
}

osThreadDef(ledTaskCallback, osPriorityNormal, 128);

int main()
{
    //Enable 2V8 and 3V3 power supply
	en2v8 = 0;
    en3v3 = 1;
    
    osThreadCreate(osThread(ledTaskCallback), NULL);
    osThreadCreate(osThread(mainTaskCallback), NULL);
   
    while (true);	 //Run OS
}
