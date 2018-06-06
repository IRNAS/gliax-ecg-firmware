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

// Handle OS later
#include "mbed.h"
#include "cmsis_os.h"
#include <ADS1298.h>
#include <Logger.h>
#include <Packetizer.h>
#include <ECGSender.h>

DigitalOut led1(PB_6);
DigitalOut led2(PB_7);

DigitalOut en2v8(PC_1);
DigitalOut en3v3(PB_8);

Serial pc(PA_2, PA_3);

//This UART might be used for Bluetooth module, we will define new UART
//for communication instead of Bluetooth
//extern "C" UART_HandleTypeDef huart2;

Packetizer dataPacketizer;
ECGSender ecgSender(dataPacketizer);

void mainTaskCallback(void const *args) 
{
	osDelay(100);
	en2v8 = 1;
	osDelay(100);

    //Initialize UART here
    pc.baud(115200);
    pc.printf("Test started\n");

	//Turn on ECG clock
	TIM_HandleTypeDef tim;
	tim.Instance = TIM1;
	HAL_TIM_PWM_Start(&tim, TIM_CHANNEL_3);

	if (!ADS1298::instance().start(&pc))
    {
        pc.printf("ADS1298 not started!!!\n");
		Logger::panic("Failed to initialize ADS1298.");
    }
	while(1)
    {
        
		//if (!Bluetooth::instance().isConnected()){
		//	ADS1298::instance().clear();
		//	OS::sleep(10);
		//	continue;
		//}
        

		if (ADS1298::instance().getAvailableData() < 512)
        {
			osDelay(10);
			continue;
		}

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
    //Run OS
    while (true);
}

