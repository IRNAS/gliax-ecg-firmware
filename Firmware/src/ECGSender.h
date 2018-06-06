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
 
#ifndef SRC_ECGSENDER_H
#define SRC_ECGSENDER_H

#include <stdint.h>

#include "Packetizer.h"
#include <BitFifo.hpp>
#include <DifferenceEcgCompressor.hpp>
#include <FlatEcgPredictor.hpp>
#include <ADS1298.h>
#include "EcgHeaderCommon.h"
#include "TestSignalGenerator.h"

class ECGSender {
public:
	ECGSender(Packetizer &packetizer);
	virtual ~ECGSender();

	void send();
private:
	uint8_t compressBuffer[ECG_COMPRESS_OUTPUT_BUFFER_SIZE];
	int32_t sampleOfChannels[ecg::DifferenceEcgCompressor::maxChannels];

	ecg::BitFifo compressFifo;
	ecg::FlatEcgPredictor ecgPredictor;
	ecg::DifferenceEcgCompressor compressor;

	Packetizer *packetizer;
	bool testSignal;

	static const int ECG_CHOP_BITS;
	float currFrequency;


	TestSignalGenerator testGenerator;
};

#endif
