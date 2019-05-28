/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 TELEMATICS LAB, Politecnico di Bari
 *
 * This file is part of 5G-simulator
 *
 * 5G-simulator is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation;
 *
 * 5G-simulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 5G-simulator; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Sergio Martiradonna <sergio.martiradonna@poliba.it>
 */

#include "ue-nb-iot-random-access.h"
#include "enb-nb-iot-random-access.h"
#include "../../../componentManagers/FrameManager.h"
#include "../../../device/UserEquipment.h"
#include "../../../flows/radio-bearer.h"

#include "../../../flows/MacQueue.h"

UeNbIoTRandomAccess::UeNbIoTRandomAccess() {

	DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
		cout << "UeNbIoTRandomAccess() " << endl;
	DEBUG_LOG_END

	m_type = RA_TYPE_NB_IOT;
	m_macEntity = NULL;
	m_backoffIndicator = 0;
}

UeNbIoTRandomAccess::UeNbIoTRandomAccess(MacEntity* mac) :
		UeNbIoTRandomAccess() {
	m_macEntity = mac;
}

UeNbIoTRandomAccess::~UeNbIoTRandomAccess() {
}

void UeNbIoTRandomAccess::StartRaProcedure() {
	DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
		cout << "StartRaProcedure() " << endl;
	DEBUG_LOG_END

	if ((m_macEntity->GetDevice()->GetNodeState() != NetworkNode::STATE_ACTIVE)) {
		if (m_RaProcedureActive == false) {
			m_RaProcedureActive = true;

			DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
				cout << "RANDOM_ACCESS START UE "
						<< m_macEntity->GetDevice()->GetIDNetworkNode() << " T "
						<< Simulator::Init()->Now() << endl;
			DEBUG_LOG_END

			SendMessage1();
		}
	} else {
		UserEquipment* ue = (UserEquipment*) m_macEntity->GetDevice();
		ue->SetLastActivity();
	}
}

void UeNbIoTRandomAccess::ReStartRaProcedure() {

	DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
		cout << "ReStartRaProcedure() " << endl;
	DEBUG_LOG_END

	if (m_RaProcedureActive == true) {
		m_RaProcedureActive = false;
		m_nbFailedAttempts++;
		if (m_nbFailedAttempts < m_maxFailedAttempts) {
			DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
				cout << "RANDOM_ACCESS RESTART UE "
						<< m_macEntity->GetDevice()->GetIDNetworkNode()
						<< " TRY " << m_nbFailedAttempts << " T "
						<< Simulator::Init()->Now() << endl;
			DEBUG_LOG_END

			Simulator::Init()->Schedule(0.0,
					&UeNbIoTRandomAccess::StartRaProcedure, this); //###
		} else {
			DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
				cout << "RANDOM_ACCESS FAIL UE "
						<< m_macEntity->GetDevice()->GetIDNetworkNode() << " T "
						<< Simulator::Init()->Now() << endl;
			DEBUG_LOG_END

			m_nbFailedAttempts = 0;
		}
	}
}

void UeNbIoTRandomAccess::SendMessage1() {

	DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
		cout << "SendMessage1() " << endl;
	DEBUG_LOG_END
	UserEquipment* ue = (UserEquipment*) m_macEntity->GetDevice();
	ENodeB* enb = ue->GetTargetNode();
	EnbMacEntity* enbMac = enb->GetMacEntity();
	EnbNbIoTRandomAccess* enbRam =
			(EnbNbIoTRandomAccess*) enbMac->GetRandomAccessManager();
	int currentTTI = FrameManager::Init()->GetTTICounter();

	if (isRachOpportunity() && (currentTTI % enbRam->m_periodicity) == 0) {
		RandomAccessPreambleIdealControlMessage* msg1 =
				new RandomAccessPreambleIdealControlMessage();

		msg1->SetSourceDevice(ue);
		msg1->SetDestinationDevice(ue->GetTargetNode());

		EnbMacEntity* enbMac = ue->GetTargetNode()->GetMacEntity();
		EnbNbIoTRandomAccess* enbRam =
				(EnbNbIoTRandomAccess*) enbMac->GetRandomAccessManager();

		int max_rar = enbRam->getNbRARs();
		msg1->SetMaxRAR(max_rar);

		ue->GetPhy()->SendIdealControlMessage(msg1);
	} else {
		Simulator::Init()->Schedule(0.001, &UeNbIoTRandomAccess::SendMessage1,
				this);
	}
}

void UeNbIoTRandomAccess::ReceiveMessage2(int msg3time) {
	DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
		cout << "ReceiveMessage2(" << msg3time << ") " << endl;
	DEBUG_LOG_END
	DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
		cout << "RANDOM_ACCESS RECEIVE_MSG2 UE "
				<< m_macEntity->GetDevice()->GetIDNetworkNode() << " T "
				<< Simulator::Init()->Now() << endl;
	DEBUG_LOG_END

	double delay = msg3time / 1000.0 - Simulator::Init()->Now();

	Simulator::Init()->Schedule(delay, &UeNbIoTRandomAccess::SendMessage3,
			this);
}

void UeNbIoTRandomAccess::SendMessage3() {
	DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
		cout << "SendMessage3()" << endl;
	DEBUG_LOG_END
	UserEquipment* ue = (UserEquipment*) m_macEntity->GetDevice();
	DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
		cout << "RANDOM_ACCESS SEND_MSG3 UE " << ue->GetIDNetworkNode() << " T "
				<< Simulator::Init()->Now() << endl;
	DEBUG_LOG_END

	m_msg3 = new RandomAccessConnectionRequestIdealControlMessage();
	m_msg3->SetSourceDevice(ue);
	m_msg3->SetDestinationDevice(ue->GetTargetNode());
	ue->GetPhy()->SendIdealControlMessage(m_msg3);
}

void UeNbIoTRandomAccess::ReceiveMessage4() {
	DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
		cout << "ReceiveMessage4()" << endl;
	DEBUG_LOG_END

	DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
		cout << "RANDOM_ACCESS RECEIVE_MSG4 UE "
				<< m_macEntity->GetDevice()->GetIDNetworkNode() << " T "
				<< Simulator::Init()->Now() << endl;
	DEBUG_LOG_END

	RrcEntity *rrc =
			m_macEntity->GetDevice()->GetProtocolStack()->GetRrcEntity();
	RrcEntity::RadioBearersContainer* bearers = rrc->GetRadioBearerContainer();
	std::vector<RadioBearer*>::iterator it = bearers->begin();
	int id =
			(*it)->GetMacQueue()->GetPacketQueue()->begin()->GetPacket()->GetID();

	DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
		cout << "RACH_WIN UE " << m_macEntity->GetDevice()->GetIDNetworkNode()
				<< " T " << Simulator::Init()->Now() << " ID " << id << endl;
	DEBUG_LOG_END

	UeMacEntity* mac = (UeMacEntity*) m_macEntity;

	mac->GetDevice()->MakeActive();
	UeRandomAccess::SetRaProcedureActive(false);
	m_nbFailedAttempts = 0;
	mac->SendSchedulingRequest();

}
