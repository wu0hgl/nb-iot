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
 * Author: Giuseppe Piro <g.piro@poliba.it>
 * Author2: Alessandro Grassi <alessandro.grassi@poliba.it>
 */

#include "enb-mac-entity.h"
#include "../packet/Packet.h"
#include "../packet/packet-burst.h"
#include "AMCModule.h"
#include "../../device/NetworkNode.h"
#include "packet-scheduler/packet-scheduler.h"
#include "packet-scheduler/downlink-packet-scheduler.h"
#include "packet-scheduler/uplink-packet-scheduler.h"
#include "../../device/UserEquipment.h"
#include "../../device/ENodeB.h"
#include "../../load-parameters.h"
#include "harq-manager.h"
#include "random-access/enb-lte-random-access.h"
#include "random-access/enb-enhanced-lte-random-access.h"
#include "../../core/eventScheduler/simulator.h"
#include "random-access/enb-nb-iot-random-access.h"

EnbMacEntity::EnbMacEntity() {
	SetAmcModule(new AMCModule());
	SetDevice(nullptr);
	m_downlinkScheduler = nullptr;
	m_uplinkScheduler = nullptr;
	m_defaultDlTxMode = 1;
	m_enbrandomaccess = new EnbLteRandomAccess(this);
}

EnbMacEntity::~EnbMacEntity() {
	delete m_downlinkScheduler;
	delete m_uplinkScheduler;
	delete m_enbrandomaccess;
	Destroy();
}

void EnbMacEntity::SetUplinkPacketScheduler(UplinkPacketScheduler* s) {
	m_uplinkScheduler = s;
}

void EnbMacEntity::SetDownlinkPacketScheduler(DownlinkPacketScheduler* s) {
	m_downlinkScheduler = s;
}

UplinkPacketScheduler*
EnbMacEntity::GetUplinkPacketScheduler(void) {
	return m_uplinkScheduler;
}

DownlinkPacketScheduler*
EnbMacEntity::GetDownlinkPacketScheduler(void) {
	return m_downlinkScheduler;
}

EnbRandomAccess*
EnbMacEntity::GetRandomAccessManager(void) {
	return m_enbrandomaccess;
}

void EnbMacEntity::SetRandomAccessManager(EnbRandomAccess* s) {
	m_enbrandomaccess = s;
}

void EnbMacEntity::SetRandomAccessType(EnbRandomAccess::RandomAccessType type) {
	delete m_enbrandomaccess;
	switch (type) {
	case EnbRandomAccess::RA_TYPE_LTE:
		m_enbrandomaccess = new EnbLteRandomAccess(this);
		break;

	case EnbRandomAccess::RA_TYPE_ENHANCED_LTE:
		m_enbrandomaccess = new EnbEnhancedLteRandomAccess(this);
		break;

	case EnbRandomAccess::RA_TYPE_NB_IOT:
		m_enbrandomaccess = new EnbNbIoTRandomAccess(this);
		break;

	default:
		cout << "error : unknown random access type " << type << endl;
		exit(1);
	}
}

void EnbMacEntity::ReceiveCqiIdealControlMessage(CqiIdealControlMessage* msg) {
	DEBUG_LOG_START_1(LTE_SIM_TEST_CQI_FEEDBACKS)
		cout << "ReceiveIdealControlMessage (MAC) from  "
				<< msg->GetSourceDevice()->GetIDNetworkNode() << " to "
				<< msg->GetDestinationDevice()->GetIDNetworkNode() << endl;
	DEBUG_LOG_END

	CqiIdealControlMessage::CqiFeedbacks *cqi = msg->GetMessage();

	UserEquipment* ue = (UserEquipment*) msg->GetSourceDevice();
	ENodeB::UserEquipmentRecord* record = ue->GetTargetNodeRecord();
	AMCModule *amc = ue->GetProtocolStack()->GetMacEntity()->GetAmcModule();

	if (record != nullptr) {
		vector<int> cqiFeedback;
		for (auto feedback : *cqi) {
			cqiFeedback.push_back(feedback.m_cqi);
		}

		DEBUG_LOG_START_1(LTE_SIM_TEST_CQI_FEEDBACKS)
			cout << "\t CQI: ";
			for (auto cqi : cqiFeedback) {
				cout << cqi << " ";
			}
			cout << endl;
		DEBUG_LOG_END

		DEBUG_LOG_START_1(LTE_SIM_AMC_MAPPING)
			cout << "\t CQI: ";
			for (auto cqi : cqiFeedback) {
				cout << cqi << " ";
			}
			cout << endl;

			cout << "\t MCS: ";
			for (auto cqi : cqiFeedback) {
				cout << amc->GetMCSFromCQI(cqi) << " ";
			}
			cout << endl;

			cout << "\t TB: ";
			for (auto cqi : cqiFeedback) {
				cout << amc->GetTBSizeFromMCS(amc->GetMCSFromCQI(cqi)) << " ";
			}
			cout << endl;
		DEBUG_LOG_END

		record->SetCQI(cqiFeedback);

	} else {
		cout << "ERROR: received cqi from unknown ue!" << endl;
	}
}

void EnbMacEntity::ReceiveIdealControlMessage(IdealControlMessage* msg) {
	switch (msg->GetMessageType()) {
	case IdealControlMessage::CQI_FEEDBACKS:
		DEBUG_LOG_START_1(LTE_SIM_TEST_CQI_FEEDBACKS)
			cout << "Receive CQI feedbacks from  "
					<< msg->GetSourceDevice()->GetIDNetworkNode() << " to "
					<< msg->GetDestinationDevice()->GetIDNetworkNode() << endl;
		DEBUG_LOG_END
		ReceiveCqiIdealControlMessage((CqiIdealControlMessage*) msg);
		break;

	case IdealControlMessage::SCHEDULING_REQUEST:
		ReceiveSchedulingRequestIdealControlMessage(
				(SchedulingRequestIdealControlMessage*) msg);
		break;

	case IdealControlMessage::RI_FEEDBACK:
		ReceiveRiIdealControlMessage((RankIndicatorIdealControlMessage*) msg);
		break;

	case IdealControlMessage::PMI_FEEDBACK:
		ReceivePmiIdealControlMessage((PmiIdealControlMessage*) msg);
		break;

	case IdealControlMessage::FULLCSI_FEEDBACK:
		ReceiveFullCsiIdealControlMessage((FullCsiIdealControlMessage*) msg);
		break;

	case IdealControlMessage::HARQ_ACK:
		ReceiveHarqAckIdealControlMessage((HarqIdealControlMessage*) msg);
		break;

	case IdealControlMessage::RA_PREAMBLE:
		DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
			cout << "PREAMBLE "
					<< ((RandomAccessPreambleIdealControlMessage*) msg)->GetPreamble()
					<< " RAR "
					<< ((RandomAccessPreambleIdealControlMessage*) msg)->GetMultipleRAR()
					<< endl;
			cout << "RANDOM_ACCESS RECEIVE_MSG1 FROM_UE "
					<< msg->GetSourceDevice()->GetIDNetworkNode() << " T "
					<< Simulator::Init()->Now() << endl;
		DEBUG_LOG_END
		ReceiveRandomAccessPreambleIdealControlMessage(
				(RandomAccessPreambleIdealControlMessage*) msg);
		break;

	case IdealControlMessage::RA_CONNECTION_REQUEST:
		ReceiveRandomAccessConnectionRequestIdealControlMessage(
				(RandomAccessConnectionRequestIdealControlMessage*) msg);
		break;

	default:
		cout
				<< "Error in EnbMacEntity::ReceiveIdealControlMessage(): unknown message type received ("
				<< msg->GetMessageType() << ")" << endl;
		exit(1);
		break;
	}

}

void EnbMacEntity::ReceiveRandomAccessPreambleIdealControlMessage(
		RandomAccessPreambleIdealControlMessage* msg) {
	GetRandomAccessManager()->ReceiveMessage1(msg);
}

void EnbMacEntity::ReceiveRandomAccessConnectionRequestIdealControlMessage(
		RandomAccessConnectionRequestIdealControlMessage* msg) {
	GetRandomAccessManager()->ReceiveMessage3(msg);
}

void EnbMacEntity::ReceiveRiIdealControlMessage(
		RankIndicatorIdealControlMessage* msg) {
	int ri = msg->GetRI();

	UserEquipment* ue = (UserEquipment*) msg->GetSourceDevice();
	ENodeB::UserEquipmentRecord* record = ue->GetTargetNodeRecord();

	if (record != nullptr) {
		DEBUG_LOG_START_1(LTE_SIM_TEST_RI_FEEDBACK)
			cout << "\t RI: " << ri << endl;
		DEBUG_LOG_END
		record->SetRI(ri);
	} else {
		cout << "ERROR: received ri from unknown ue!" << endl;
	}
}

void EnbMacEntity::ReceivePmiIdealControlMessage(PmiIdealControlMessage* msg) {
	vector<vector<int> > pmi = msg->GetPMI();

	UserEquipment* ue = (UserEquipment*) msg->GetSourceDevice();
	ENodeB::UserEquipmentRecord* record = ue->GetTargetNodeRecord();

	if (record != nullptr) {
		DEBUG_LOG_START_1(LTE_SIM_TEST_PMI_FEEDBACK)
			cout << "\t PMI: " << pmi << endl;
		DEBUG_LOG_END
		record->SetPMI(pmi);
	} else {
		cout << "ERROR: received pmi from unknown ue!" << endl;
	}
}

void EnbMacEntity::ReceiveFullCsiIdealControlMessage(
		FullCsiIdealControlMessage* msg) {
	vector<shared_ptr<arma::cx_fmat> > channelMatrix = msg->GetChannelMatrix();

	UserEquipment* ue = (UserEquipment*) msg->GetSourceDevice();
	ENodeB::UserEquipmentRecord* record = ue->GetTargetNodeRecord();

	if (record != nullptr) {
		record->SetChannelMatrix(channelMatrix);
	} else {
		cout << "ERROR: received csi from unknown ue!" << endl;
	}
}

void EnbMacEntity::SendPdcchMapIdealControlMessage(
		PdcchMapIdealControlMessage* msg) {
}

void EnbMacEntity::ReceiveSchedulingRequestIdealControlMessage(
		SchedulingRequestIdealControlMessage* msg) {
	UserEquipment* ue = (UserEquipment*) msg->GetSourceDevice();
	ENodeB::UserEquipmentRecord* record = ue->GetTargetNodeRecord();

	int bufferStatusReport = msg->GetBufferStatusReport();

	if (record != nullptr) {
		record->SetSchedulingRequest(bufferStatusReport);
	} else {
		cout << "ERROR: received cqi from unknown ue!" << endl;
	}
}

void EnbMacEntity::ReceiveHarqAckIdealControlMessage(
		HarqIdealControlMessage *msg) {
	UserEquipment* ue = (UserEquipment*) msg->GetSourceDevice();
	ENodeB::UserEquipmentRecord* record = ue->GetTargetNodeRecord();
	HarqManager* harq = record->GetHarqManager();
	harq->ReceiveAck(msg->GetAck(), msg->GetPid());
}

void EnbMacEntity::SetDefaultDlTxMode(int txmode) {
	m_defaultDlTxMode = txmode;
}

int EnbMacEntity::GetDefaultDlTxMode(void) {
	return m_defaultDlTxMode;
}

ENodeB*
EnbMacEntity::GetDevice(void) {
	NetworkNode* node = ((MacEntity*) this)->GetDevice();
	return (ENodeB*) node;
}
