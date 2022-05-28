/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/
#define _USE_MATH_DEFINES

#include "../stdafx.h"
#include "../FamiTracker.h"
#include "../Settings.h"
#include "../Common.h"
#include "APU.h"
#include "N163.h"
#include "../RegisterState.h"		// // //

#include <cstdint>
#include <cmath>

//
// Namco 163
//

// N163 interface, emulation is in Namco163Sound.h

CN163::CN163()
{
	m_pRegisterLogger->AddRegisterRange(0x00, 0x7F);		// // //
	Reset();
}

CN163::~CN163()
{
}

void CN163::Reset()
{
	m_N163.Reset();

	m_SynthN163.clear();
	m_BlipN163.clear();
}

void CN163::UpdateFilter(blip_eq_t eq)
{
	m_BlipN163.set_sample_rate(eq.sample_rate);
	m_SynthN163.treble_eq(eq);
	m_BlipN163.bass_freq(0);
	m_CutoffHz = 12000;
	m_N163.SetMixing(m_bOldMixing);
	RecomputeN163Filter();
}

void CN163::SetClockRate(uint32_t Rate)
{
	m_BlipN163.clock_rate(Rate);
}

void CN163::Write(uint16_t Address, uint8_t Value)
{
	m_N163.WriteRegister(Address, Value);
}

uint8_t CN163::Read(uint16_t Address, bool &Mapped)
{
	// Addresses for N163
	Mapped = (0x4800 <= Address && Address <= 0x4FFF);
	return m_N163.ReadRegister(Address);
}

void CN163::Process(uint32_t Time, Blip_Buffer& Output)
{
	uint32_t now = 0;

	while (true) {
		//assert(now <= Time);
		if (now >= Time)
			break;

		//auto tmp = m_FDS.ClockAudioMaxSkip();
		//auto clock_skip = std::min(tmp, Time - now);
		//if (clock_skip > 0) {
		//	m_FDS.SkipClockAudio(clock_skip);
		//	now += clock_skip;
		//}

		//if (tmp < (1 << 24))
		//	assert(tmp - m_FDS.ClockAudioMaxSkip() == clock_skip);
		//assert(now <= Time);
		//if (now >= Time)
		//	break;

		m_N163.ClockAudio();
		for (int i = 0; i < 8; i++)
			m_ChannelLevels[i].update(m_N163._channelOutput[i]);

		auto master_out = m_N163.UpdateOutputLevel();
		m_SynthN163.update(m_iTime + now, (int)master_out, &m_BlipN163);
		now++;
	}

	m_iTime += Time;

}

void CN163::EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer)
{
	m_BlipN163.end_frame(m_iTime);

	ASSERT(size_t(m_BlipN163.samples_avail()) <= TempBuffer.size());

	auto nsamp_read = m_BlipN163.read_samples(TempBuffer.data(), m_BlipN163.samples_avail());

	auto unfilteredData = TempBuffer.subspan(0, nsamp_read);

	for (auto& amplitude : unfilteredData) {
		float out = m_lowPassState + m_alpha * (float(amplitude) - m_lowPassState);
		amplitude = (int16_t)roundf(out);
		m_lowPassState = out + 1e-18f;  // prevent denormal numbers
	}

	Output.mix_samples_raw(unfilteredData.data(), unfilteredData.size());

	m_iTime = 0;
}

double CN163::GetFreq(int Channel) const
{
	return m_N163.GetFrequency(Channel);
}

int CN163::GetChannelLevel(int Channel)
{
	ASSERT(0 <= Channel && Channel < 8);
	if (0 <= Channel && Channel < 8) {
		return m_ChannelLevels[Channel].getLevel();
	}
	return 0;
}

int CN163::GetChannelLevelRange(int Channel) const
{
	ASSERT(0 <= Channel && Channel < 8);
	if (0 <= Channel && Channel < 8) {
		return 255; // (4 bit sample * 4 bit volume) - 1
	}
	return 1;
}

void CN163::UpdateN163Filter(int CutoffHz, bool Multiplex)
{
	m_CutoffHz = CutoffHz;
	RecomputeN163Filter();
	SetMixingMethod(Multiplex);
}

void CN163::UpdateMixLevel(double v)
{
	m_SynthN163.volume(v * 1.1f, 1600);
}

void CN163::Log(uint16_t Address, uint8_t Value)		// // //
{
	switch (Address) {
	case 0xF800:
		m_pRegisterLogger->SetPort(Value & 0x7F);
		m_pRegisterLogger->SetAutoincrement((Value & 0x80) != 0);
		break;
	case 0x4800:
		m_pRegisterLogger->Write(Value);
		break;
	}
}

void CN163::SetMixingMethod(bool bLinear)		// // //
{
	m_bOldMixing = bLinear;
}

void CN163::RecomputeN163Filter()
{
	// Code taken from FDS.cpp
	auto sampleRate_hz = float(m_BlipN163.sample_rate());
	auto cutoff_rad = M_PI * 2 * (float)m_CutoffHz / sampleRate_hz;

	// Wonder if this'll get messed up due to its large cutoff frequency
	m_alpha = 1 - (float)std::exp(-cutoff_rad);
}