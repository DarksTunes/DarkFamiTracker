/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "stdafx.h"
#include "FamiTracker.h" // theApp.getSoundGenerator()
#include "FTMComponentInterface.h"		// // //
#include "SequenceCollection.h"		// // //
#include "SequenceManager.h"		// // //
#include "APU/Types.h"
#include "FamiTrackerTypes.h"
#include "SoundGen.h"

#include "Instrument.h"
#include "Sequence.h"
#include "ChannelHandlerInterface.h"
#include "InstHandler.h"
#include "SeqInstHandler.h"

/*
 * Class CSeqInstHandler
 */

CSeqInstHandler::CSeqInstHandler(CChannelHandlerInterface *pInterface, int Vol, int Duty) :
	CInstHandler(pInterface, Vol),
	m_iDefaultDuty(Duty)
{
	for (size_t i = 0; i < sizeof(m_pSequence) / sizeof(CSequence*); i++)
		ClearSequence(i);
}

void CSeqInstHandler::LoadInstrument(CInstrument *pInst)
{
	m_pInstrument = pInst;
	CSeqInstrument *pSeqInst = dynamic_cast<CSeqInstrument*>(pInst);
	CSequenceManager *pManager = theApp.GetSoundGenerator()->GetDocumentInterface()->GetSequenceManager(pInst->GetType());
	if (pSeqInst == nullptr) return;
	for (size_t i = 0; i < sizeof(m_pSequence) / sizeof(CSequence*); i++) {
		const CSequence *pSequence = pManager->GetCollection(i)->GetSequence(pSeqInst->GetSeqIndex(i));
		bool Enable = pSeqInst->GetSeqEnable(i) == SEQ_STATE_RUNNING;
		if (!Enable)
			ClearSequence(i);
		else if (pSequence != m_pSequence[i] || m_iSeqState[i] == SEQ_STATE_DISABLED)
			SetupSequence(i, pSequence);
	}
}

void CSeqInstHandler::TriggerInstrument()
{
	for (size_t i = 0; i < sizeof(m_pSequence) / sizeof(CInstrument*); i++) if (m_pSequence[i] != nullptr) {
		m_iSeqState[i] = SEQ_STATE_RUNNING;
		m_iSeqPointer[i] = 0;
	}
	m_iVolume = m_iDefaultVolume;
	m_iNoteOffset = 0;
	m_iPitchOffset = 0;
	m_iDutyParam = m_iDefaultDuty;
}

void CSeqInstHandler::ReleaseInstrument()
{
	if (m_pInterface->IsReleasing()) return;
	for (size_t i = 0; i < sizeof(m_pSequence) / sizeof(CInstrument*); i++)
		if (m_pSequence[i] != nullptr && (m_iSeqState[i] == SEQ_STATE_RUNNING || m_iSeqState[i] == SEQ_STATE_END)) {
			int ReleasePoint = m_pSequence[i]->GetReleasePoint();
			if (ReleasePoint != -1) {
				m_iSeqPointer[i] = ReleasePoint;
				m_iSeqState[i] = SEQ_STATE_RUNNING;
			}
		}
}

void CSeqInstHandler::UpdateInstrument()
{
	if (!m_pInterface->IsActive()) return;
	for (size_t i = 0; i < sizeof(m_pSequence) / sizeof(CSequence*); i++) {
		if (m_pSequence[i] == nullptr || m_pSequence[i]->GetItemCount() == 0) continue;
		int Value = m_pSequence[i]->GetItem(m_iSeqPointer[i]);
		switch (m_iSeqState[i]) {
		case SEQ_STATE_RUNNING:
			switch (i) {
			// Volume modifier
			case SEQ_VOLUME:
				m_pInterface->SetVolume(Value);
				break;
			// Arpeggiator
			case SEQ_ARPEGGIO:
				switch (m_pSequence[i]->GetSetting()) {
				case SETTING_ARP_ABSOLUTE:
					m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote() + Value));
					break;
				case SETTING_ARP_FIXED:
					m_pInterface->SetPeriod(m_pInterface->TriggerNote(Value));
					break;
				case SETTING_ARP_RELATIVE:
					m_pInterface->SetNote(m_pInterface->GetNote() + Value);
					m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote()));
					break;
				case SETTING_ARP_SCHEME: // // //
					if (Value < 0) Value += 256;
					int lim = Value % 0x40, scheme = Value / 0x40;
					if (lim > 36)
						lim -= 64;
					{
						unsigned char Param = m_pInterface->GetArpParam();
						switch (scheme) {
						case 0: break;
						case 1: lim += Param >> 4;   break;
						case 2: lim += Param & 0x0F; break;
						case 3: lim -= Param & 0x0F; break;
						}
					}
					m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote() + lim));
					break;
				}
				break;
			// Pitch
			case SEQ_PITCH:
				m_pInterface->SetPeriod(m_pInterface->GetPeriod() + Value);
				break;
			// Hi-pitch
			case SEQ_HIPITCH:
				m_pInterface->SetPeriod(m_pInterface->GetPeriod() + (Value << 4));
				break;
			// Duty cycling
			case SEQ_DUTYCYCLE:
				m_pInterface->SetDutyPeriod(Value);
				break;
			}

			{
				++m_iSeqPointer[i];
				int Release = m_pSequence[i]->GetReleasePoint();
				int Items = m_pSequence[i]->GetItemCount();
				int Loop = m_pSequence[i]->GetLoopPoint();
				if (m_iSeqPointer[i] == (Release + 1) || m_iSeqPointer[i] >= Items) {
					// End point reached
					if (Loop != -1 && !(m_pInterface->IsReleasing() && Release != -1) && Loop < Release) {		// // //
						m_iSeqPointer[i] = Loop;
					}
					else if (m_iSeqPointer[i] >= Items) {
						// End of sequence 
						if (Loop >= Release && Loop != -1)		// // //
							m_iSeqPointer[i] = Loop;
						else
							m_iSeqState[i] = SEQ_STATE_END;
					}
					else if (!m_pInterface->IsReleasing()) {
						// Waiting for release
						--m_iSeqPointer[i];
					}
				}
				theApp.GetSoundGenerator()->SetSequencePlayPos(m_pSequence[i], m_iSeqPointer[i]);
			}
			break;

		case SEQ_STATE_END:
			switch (i) {
			case SEQ_ARPEGGIO:
				if (m_pSequence[i]->GetSetting() == SETTING_ARP_FIXED)
					m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote()));
				break;
			}
			m_iSeqState[i] = SEQ_STATE_HALT;
			theApp.GetSoundGenerator()->SetSequencePlayPos(m_pSequence[i], -1);
			break;

		case SEQ_STATE_HALT:
		case SEQ_STATE_DISABLED:
			break;
		}
	}
}

void CSeqInstHandler::SetupSequence(int Index, const CSequence *pSequence)
{
	m_iSeqState[Index]	 = SEQ_STATE_RUNNING;
	m_iSeqPointer[Index] = 0;
	m_pSequence[Index]	 = pSequence;
}

void CSeqInstHandler::ClearSequence(int Index)
{
	m_iSeqState[Index]	 = SEQ_STATE_DISABLED;
	m_iSeqPointer[Index] = 0;
	m_pSequence[Index]	 = nullptr;
}