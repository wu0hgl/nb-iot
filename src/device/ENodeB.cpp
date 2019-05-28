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
 */


#include "NetworkNode.h"
#include "UserEquipment.h"
#include "ENodeB.h"
#include "Gateway.h"
#include "../protocolStack/mac/packet-scheduler/packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/mt-uplink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-pf-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-mt-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-mlwdf-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-exp-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-fls-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/exp-rule-downlink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/log-rule-downlink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/dl-rr-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/enhanced-uplink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/roundrobin-uplink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/nb-roundrobin-uplink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/nb-fifo-uplink-packet-scheduler.h"
#include "../protocolStack/mac/packet-scheduler/nb-uplink-packet-scheduler.h"
#include "../phy/enb-lte-phy.h"
#include "../core/spectrum/bandwidth-manager.h"
#include "../protocolStack/packet/packet-burst.h"
#include "../protocolStack/mac/harq-manager.h"
#include "../componentManagers/FrameManager.h"
#include "../protocolStack/mac/random-access/enb-random-access.h"

ENodeB::ENodeB (int idElement,
                Cell *cell)
    : ENodeB::ENodeB (idElement,
                      cell,
                      cell->GetCellCenterPosition ()->GetCoordinateX (),
                      cell->GetCellCenterPosition ()->GetCoordinateY (),
                      25) // default value for urban macro-cell scenario
{}

ENodeB::ENodeB (int idElement,
                Cell *cell,
                double posx,
                double posy)
    : ENodeB::ENodeB (idElement,
                      cell,
                      posx,
                      posy,
                      25) // default value for urban macro-cell scenario
{}

ENodeB::ENodeB (int idElement,
                Cell *cell,
                double posx,
                double posy,
                double posz)
{
  SetIDNetworkNode (idElement);
  SetNodeType(NetworkNode::TYPE_ENODEB);
  SetCell (cell);

  CartesianCoordinates *position = new CartesianCoordinates(posx, posy, posz);
  Mobility* m = new ConstantPosition ();
  m->SetAbsolutePosition (position);
  SetMobilityModel (m);
  delete position;

  m_userEquipmentRecords = new UserEquipmentRecords;

  EnbLtePhy *phy = new EnbLtePhy ();
  phy->SetDevice(this);
  SetPhy (phy);

  ProtocolStack *stack = new ProtocolStack (this);
  SetProtocolStack (stack);

  Classifier *classifier = new Classifier ();
  classifier->SetDevice (this);
  SetClassifier (classifier);
}

ENodeB::~ENodeB()
{
  Destroy ();
  m_userEquipmentRecords->clear();
  delete m_userEquipmentRecords;
}

void
ENodeB::RegisterUserEquipment (UserEquipment *UE)
{
  UserEquipmentRecord *record = new UserEquipmentRecord (UE);
  UserEquipmentRecords *UERs = GetUserEquipmentRecords ();
  UserEquipmentRecords::iterator it, ins_pos;
  ins_pos = UERs->end();
  if (UERs->size()>0)
    {
      for (it=UERs->end()-1; it>=UERs->begin(); it--)
        {
          // UEs should be inserted before multicast destinations
          if ( it == UERs->begin() )
            {
              ins_pos = it;
              break;
            }
          if ( (*it)->GetUE()->GetNodeType()==NetworkNode::TYPE_MULTICAST_DESTINATION )
            {
              ins_pos = it;
            }
          else
            {
              break;
            }
        }
    }

  UERs->insert(ins_pos,record);
  UE->SetTargetNodeRecord (record);
}

void
ENodeB::DeleteUserEquipment (UserEquipment *UE)
{
  UserEquipmentRecords *new_records = new UserEquipmentRecords ();

  for (auto record : *GetUserEquipmentRecords())
    {
      if (record->GetUE ()->GetIDNetworkNode () != UE->GetIDNetworkNode ())
        {
          //records->erase(iter);
          //break;
          new_records->push_back (record);
        }
      else
        {
          if (UE->GetTargetNodeRecord() == record)
            {
              UE->SetTargetNodeRecord (NULL);
            }
          delete record;
        }
    }

  m_userEquipmentRecords->clear ();
  delete m_userEquipmentRecords;
  m_userEquipmentRecords = new_records;
}

int
ENodeB::GetNbOfUserEquipmentRecords (void)
{
  return GetUserEquipmentRecords ()->size();
}

void
ENodeB::CreateUserEquipmentRecords (void)
{
  m_userEquipmentRecords = new UserEquipmentRecords ();
}

void
ENodeB::DeleteUserEquipmentRecords (void)
{
  m_userEquipmentRecords->clear ();
  delete m_userEquipmentRecords;
}

ENodeB::UserEquipmentRecords*
ENodeB::GetUserEquipmentRecords (void)
{
  return m_userEquipmentRecords;
}

ENodeB::UserEquipmentRecord*
ENodeB::GetUserEquipmentRecord (int idUE)
{
  for (auto record : *GetUserEquipmentRecords())
    {
      if (record->GetUE ()->
          GetIDNetworkNode () == idUE)
        {
          return record;
        }
    }
  return nullptr;
}

EnbMacEntity*
ENodeB::GetMacEntity(void) const
{
  return (EnbMacEntity*)GetProtocolStack()->GetMacEntity();
}

ENodeB::UserEquipmentRecord::UserEquipmentRecord ()
{
  m_UE = nullptr;
  //Create initial CQI values:
  m_cqiAvailable = false;
  m_cqiFeedback.clear ();
  m_uplinkChannelStatusIndicator.clear ();
  m_schedulingRequest = 0;
  m_averageSchedulingGrants = 1;
  m_ulMcs = -1;
  SetHarqManager (new HarqManager ());
}

ENodeB::UserEquipmentRecord::~UserEquipmentRecord ()
{
  m_cqiFeedback.clear ();
  m_uplinkChannelStatusIndicator.clear();
  if (m_harqManager != nullptr)
    {
      delete m_harqManager;
    }
}

ENodeB::UserEquipmentRecord::UserEquipmentRecord (UserEquipment *UE)
{
  m_UE = UE;
  BandwidthManager *s = m_UE->GetPhy ()->GetBandwidthManager ();

  int nbRbs = s->GetDlSubChannels ().size ();
  m_cqiFeedback.clear ();
  for (int i = 0; i < nbRbs; i++ )
    {
      m_cqiFeedback.push_back (10);
    }

  nbRbs = s->GetUlSubChannels ().size ();
  m_uplinkChannelStatusIndicator.clear ();
  for (int i = 0; i < nbRbs; i++ )
    {
      m_uplinkChannelStatusIndicator.push_back (10.);
    }

  m_schedulingRequest = 0;
  m_ulMcs = -1;						// by zyb
  m_averageSchedulingGrants = 1;
  m_DlTxMode = UE->GetTargetNode()->GetMacEntity ()->GetDefaultDlTxMode ();
  if (UE->GetNodeType() == NetworkNode::TYPE_MULTICAST_DESTINATION
      || UE->GetMacEntity()->GetHarqManager()==nullptr)
    {
      SetHarqManager (nullptr);
    }
  else
    {
      SetHarqManager (new HarqManager (UE));
    }
  if (FrameManager::Init()->MbsfnEnabled()==true && UE->GetNodeType()==NetworkNode::TYPE_MULTICAST_DESTINATION)
    {
      m_DlTxMode = 1;
    }
}

void
ENodeB::UserEquipmentRecord::SetUE (UserEquipment *UE)
{
  m_UE = UE;
}

UserEquipment*
ENodeB::UserEquipmentRecord::GetUE (void) const
{
  return m_UE;
}

bool
ENodeB::UserEquipmentRecord::CqiAvailable()
{
  return m_cqiAvailable;
}

void
ENodeB::UserEquipmentRecord::SetCQI (vector<int> cqi)
{
  m_cqiAvailable = true;
  m_cqiFeedback = cqi;
}

vector<int>
ENodeB::UserEquipmentRecord::GetCQI (void) const
{
  return m_cqiFeedback;
}

void
ENodeB::UserEquipmentRecord::SetRI(int ri)
{
  m_riFeedback = ri;
}

int
ENodeB::UserEquipmentRecord::GetRI (void) const
{
  return m_riFeedback;
}

void
ENodeB::UserEquipmentRecord::SetPMI(vector< vector<int> > pmi)
{
  m_pmiFeedback = pmi;
}

vector< vector<int> >
ENodeB::UserEquipmentRecord::GetPMI (void) const
{
  return m_pmiFeedback;
}

void
ENodeB::UserEquipmentRecord::SetChannelMatrix(vector< shared_ptr<arma::cx_fmat> > channelMatrix)
{
  m_channelMatrix = channelMatrix;
}

vector< shared_ptr<arma::cx_fmat> >
ENodeB::UserEquipmentRecord::GetChannelMatrix(void)
{
  return m_channelMatrix;
}


void
ENodeB::UserEquipmentRecord::SetDlTxMode(int txMode)
{
  m_DlTxMode = txMode;
}

int
ENodeB::UserEquipmentRecord::GetDlTxMode()
{
  return m_DlTxMode;
}

void
ENodeB::UserEquipmentRecord::SetHarqManager (HarqManager* harqManager)
{
  m_harqManager = harqManager;
}

HarqManager*
ENodeB::UserEquipmentRecord::GetHarqManager (void)
{
  return m_harqManager;
}

int
ENodeB::UserEquipmentRecord::GetSchedulingRequest (void)
{
  return m_schedulingRequest;
}

void
ENodeB::UserEquipmentRecord::SetSchedulingRequest (int r)
{
  m_schedulingRequest = r;
}

void
ENodeB::UserEquipmentRecord::UpdateSchedulingGrants (int b)
{
  m_averageSchedulingGrants = (0.9 * m_averageSchedulingGrants) + (0.1 * b);
}

int
ENodeB::UserEquipmentRecord::GetSchedulingGrants (void)
{
  return m_averageSchedulingGrants;
}

void
ENodeB::UserEquipmentRecord::SetUlMcs (int mcs)
{
  m_ulMcs = mcs;
}

int
ENodeB::UserEquipmentRecord::GetUlMcs (void)
{
  return m_ulMcs;
}

void
ENodeB::UserEquipmentRecord::SetUplinkChannelStatusIndicator (vector<double> vet)
{
  m_uplinkChannelStatusIndicator = vet;
}

vector<double>
ENodeB::UserEquipmentRecord::GetUplinkChannelStatusIndicator (void) const
{
  return m_uplinkChannelStatusIndicator;
}

void
ENodeB::SetDLScheduler (ENodeB::DLSchedulerType type)
{
  EnbMacEntity *mac = GetMacEntity ();
  DownlinkPacketScheduler *scheduler;
  switch (type)
    {
    case ENodeB::DLScheduler_TYPE_PROPORTIONAL_FAIR:
      scheduler = new  DL_PF_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case ENodeB::DLScheduler_TYPE_FLS:
      scheduler = new  DL_FLS_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case ENodeB::DLScheduler_TYPE_EXP:
      scheduler = new  DL_EXP_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case ENodeB::DLScheduler_TYPE_MLWDF:
      scheduler = new  DL_MLWDF_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case ENodeB::DLScheduler_EXP_RULE:
      scheduler = new  ExpRuleDownlinkPacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case ENodeB::DLScheduler_LOG_RULE:
      scheduler = new  LogRuleDownlinkPacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case ENodeB::DLScheduler_TYPE_MAXIMUM_THROUGHPUT:
      scheduler = new  DL_MT_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    case ENodeB::DLScheduler_TYPE_ROUND_ROBIN:
      scheduler = new  DL_RR_PacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetDownlinkPacketScheduler (scheduler);
      break;

    default:
      cout << "ERROR: invalid scheduler type" << endl;
      exit(1);
      break;
    }
}

DownlinkPacketScheduler*
ENodeB::GetDLScheduler (void) const
{
  return GetMacEntity ()->GetDownlinkPacketScheduler ();
}

void
ENodeB::SetULScheduler (ULSchedulerType type)
{
  EnbMacEntity *mac = GetMacEntity ();
  UplinkPacketScheduler *scheduler;
  switch (type)
    {
    case ENodeB::ULScheduler_TYPE_MAXIMUM_THROUGHPUT:
      scheduler = new MaximumThroughputUplinkPacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetUplinkPacketScheduler (scheduler);
      break;
    case ENodeB::ULScheduler_TYPE_FME:
      scheduler = new EnhancedUplinkPacketScheduler();
      scheduler->SetMacEntity (mac);
      mac->SetUplinkPacketScheduler (scheduler);
      break;
    case ENodeB::ULScheduler_TYPE_ROUNDROBIN:
      scheduler = new RoundRobinUplinkPacketScheduler ();
      scheduler->SetMacEntity (mac);
      mac->SetUplinkPacketScheduler (scheduler);
      break;
    case ENodeB::ULScheduler_TYPE_NB_IOT_FIFO:
      scheduler = new nbFifoUplinkPacketScheduler (mac);
      mac->SetUplinkPacketScheduler (scheduler);
      break;
    case ENodeB::ULScheduler_TYPE_NB_IOT_ROUNDROBIN:
      scheduler = new nbRoundRobinUplinkPacketScheduler (mac);
      mac->SetUplinkPacketScheduler (scheduler);
      break;

    default:
      cout << "ERROR: invalid scheduler type" << endl;
      exit(1);
    }
}

UplinkPacketScheduler*
ENodeB::GetULScheduler (void) const
{
  return GetMacEntity ()->GetUplinkPacketScheduler ();
}

void
ENodeB::ResourceBlocksAllocation (void)
{
  DownlinkResourceBlockAllocation ();
  UplinkResourceBlockAllocation ();
}

void
ENodeB::UplinkResourceBlockAllocation (void)
{
  if (GetULScheduler () != nullptr && GetNbOfUserEquipmentRecords () > 0)
    {
      GetULScheduler ()->Schedule();
    }
}

void
ENodeB::DownlinkResourceBlockAllocation (void)
{
  if (GetDLScheduler () != nullptr && GetNbOfUserEquipmentRecords () > 0)
    {
      GetDLScheduler ()->Schedule();
    }
  else
    {
      //send only reference symbols
      //PacketBurst *pb = new PacketBurst ();
      //SendPacketBurst (pb);
    }
}

void
ENodeB::SetRandomAccessType(EnbRandomAccess::RandomAccessType type)
{
  GetMacEntity ()->SetRandomAccessType(type);
}


//Debug
void
ENodeB::Print (void)
{
  cout << " ENodeB object:"
            "\n\t m_idNetworkNode = " << GetIDNetworkNode () <<
            "\n\t m_idCell = " << GetCell ()->GetIdCell () <<
            "\n\t Served Users: " <<
            endl;

  for (auto record : *GetUserEquipmentRecords())
    {
      cout << "\t\t idUE = " << record->GetUE ()->
                GetIDNetworkNode () << endl;
    }
}
