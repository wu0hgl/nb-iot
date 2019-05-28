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

#include "enb-nb-iot-random-access.h"
#include "../../../componentManagers/FrameManager.h"
#include "../../../core/eventScheduler/simulator.h"
#include "../../../device/ENodeB.h"
#include "ue-nb-iot-random-access.h"
#include "../enb-mac-entity.h"
#include "../ue-mac-entity.h"
#include "../../../core/idealMessages/ideal-control-messages.h"
#include "../../../device/NetworkNode.h"
#include "../../../phy/lte-phy.h"
#include "../../../core/spectrum/bandwidth-manager.h"
#include "../../../device/UserEquipment.h"
#include "../../../load-parameters.h"

EnbNbIoTRandomAccess::EnbNbIoTRandomAccess()
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::EnbNbIoTRandomAccess() " << endl;
DEBUG_LOG_END
  m_PreambleFormat = Preamble_FORMAT_0; //#
  m_type = RA_TYPE_NB_IOT;
  m_lastRachOpportunity = 0; //#
  m_periodicity = 320; // in SFs
  m_PreambleDuration = 6; // SFs
  m_macEntity = nullptr;
  m_responseWindowDelay = 3; // SFs
  m_responseWindowDuration = 48; // SFs
  m_nbRARs = 1; //#
  m_maxCCCHUsage = (float)(2*m_nbRARs+1)/((m_nbRARs+1)*2);

  m_RachReservation = Simulator::Init()->Schedule(0.00, &EnbNbIoTRandomAccess::SetRachReservedSubChannels, this);
  m_RarScheduling = Simulator::Init()->Schedule(0.000, &EnbNbIoTRandomAccess::CheckCollisions, this);
}


EnbNbIoTRandomAccess::EnbNbIoTRandomAccess(MacEntity* mac) : EnbNbIoTRandomAccess()
{
  m_macEntity = mac;
}


EnbNbIoTRandomAccess::~EnbNbIoTRandomAccess()
{
  m_RachReservation->MarkDeleted();
  m_RarScheduling->MarkDeleted();
}


void
EnbNbIoTRandomAccess::SetRachReservedSubChannels()
{
  int currentSF = FrameManager::Init()->GetTTICounter();
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::SetRachReservedSubChannels() - SF " << currentSF
       <<" T " << Simulator::Init()->Now()
       << endl;
DEBUG_LOG_END

  int predictionWindow = m_PreambleDuration + m_responseWindowDelay + 2*m_responseWindowDuration;

  //m_tonesReservedForRach.deleteOldElements(currentSF);

DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
  cout << "SF "<< currentSF << " RANDOM_ACCESS RACH session planned at SF(s) ";
DEBUG_LOG_END
  for (int sf=currentSF; sf<=currentSF+predictionWindow; sf++)
    {
      if (sf % m_periodicity < (m_PreambleDuration))
        {
          if ( ! m_tonesReservedForRach.elementsExist(sf) )
            {
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
              cout << sf << " ";
DEBUG_LOG_END
              for(int i=0; i<48; i++) //#
                {
                  m_tonesReservedForRach.addElement(sf,i);
                }
            }
        }
    }
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
  cout << endl ;
DEBUG_LOG_END

  if (m_tonesReservedForRach.elementsExist(currentSF) )
    {
      m_RachReservation = Simulator::Init()->Schedule(m_periodicity/1000.0, &EnbNbIoTRandomAccess::SetRachReservedSubChannels,this);
    }
  else
    {
      m_RachReservation = Simulator::Init()->Schedule(0.001, &EnbNbIoTRandomAccess::SetRachReservedSubChannels,this);
    }
}

void
EnbNbIoTRandomAccess::ReceiveMessage1( RandomAccessPreambleIdealControlMessage *msg)
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::ReceiveMessage1(msg) " << endl;
DEBUG_LOG_END

  int tTx = msg->GetTimeTx();

  preambleMessage idU;
  idU.ue = msg->GetSourceDevice();
  idU.preamble = msg->GetPreamble();

  rachSession rs;

  if(m_savedRachSessions.find(tTx) != m_savedRachSessions.end())
    {
      rs = (*m_savedRachSessions.find(tTx)).second;
      m_savedRachSessions.erase(m_savedRachSessions.find(tTx));
    }

  rs.push_back(idU);

  pair <int, rachSession> timeAndRachSession (tTx, rs);

  m_savedRachSessions.insert(timeAndRachSession);
}


void
EnbNbIoTRandomAccess::CheckCollisions()
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::CheckCollisions(msg) " << endl;
DEBUG_LOG_END

  int currentSF = FrameManager::Init()->GetTTICounter();
  int nbOfRBs = m_macEntity->GetDevice ()->GetPhy ()->GetBandwidthManager ()->GetUlSubChannels ().size ();

  int verifyTime = currentSF - m_responseWindowDelay - m_PreambleDuration + 1;

  std::map <int, rachSession>::iterator it;

  it = m_savedRachSessions.find(verifyTime);

  vector<int>  collidedId;
  collidedId.clear();

  if(it != m_savedRachSessions.end())
    {
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
      cout << "T "<< Simulator::Init()->Now()
           <<" SF " << currentSF
           << " - RANDOM_ACCESS RACH session found at SF " << verifyTime
           << endl;
DEBUG_LOG_END

      rachSession messages = (*it).second;
      deque<int> availableSFs;
      availableSFs.clear();


DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
      cout << "Available SFs for Message 3 TX: " ;
DEBUG_LOG_END

      for (int targetTime=currentSF+1; targetTime<=currentSF+m_responseWindowDuration; targetTime++)
        {
          if (! m_tonesReservedForRach.elementsExist(targetTime) )
            {
              availableSFs.push_back(targetTime);
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
              cout << targetTime << " ";
DEBUG_LOG_END
            }
        }

DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
      cout << endl;
DEBUG_LOG_END

      for (auto k : messages)
        {
          bool collision = false;
          for (auto j : messages)
            {
              if (k.preamble == j.preamble && k.ue != j.ue)
                {
                      collision = true;
                      if (find(collidedId.begin(), collidedId.end(), k.ue->GetIDNetworkNode()) == collidedId.end())
                        {
                          collidedId.push_back(k.ue->GetIDNetworkNode());

                          DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
                          cout << "COLLISION UE " << k.ue->GetIDNetworkNode() << " UE " << j.ue->GetIDNetworkNode() << " T " << verifyTime << endl;
                          DEBUG_LOG_END

                          UeMacEntity* ueMac = (UeMacEntity*) k.ue->GetProtocolStack()->GetMacEntity();
                          UeNbIoTRandomAccess* ueRam = (UeNbIoTRandomAccess*) ueMac->GetRandomAccessManager();
                          Simulator::Init()->Schedule(0.5*m_periodicity/1000.0, &UeNbIoTRandomAccess::ReStartRaProcedure, ueRam);
                        }
                }
            }
          if (collision == false)
            {
              if (availableSFs.empty())
                {
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
                  cout << "RANDOM_ACCESS SEND_MSG2_FAILED UE " << k.ue->GetIDNetworkNode()
                       << " T " << Simulator::Init()->Now()
                       << endl;
DEBUG_LOG_END
                }
              else
                {
                  rarElement e;
                  e.msg1Time = verifyTime;
                  e.msg3Time = availableSFs.at(0);;
                  e.ue = k.ue;
                  m_rarQueue.push(e);
                  availableSFs.pop_front();
                }
            }
        }
      m_savedRachSessions.erase(verifyTime);
      cout << "RACH INFO SF " << verifyTime
           << " WIN " << messages.size() - collidedId.size()
           << " COLLISIONS " <<  collidedId.size()
           << " TOT " << messages.size()
           << endl;
    }
  if ( !m_rarQueue.empty() )
    {
      SendResponses();
    }
  m_RarScheduling = Simulator::Init()->Schedule(0.001, &EnbNbIoTRandomAccess::CheckCollisions, this);
}

void
EnbNbIoTRandomAccess::SendResponses()
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::SendResponses() " << endl;
DEBUG_LOG_END
  while( ! m_rarQueue.empty())
    {
      rarElement e = m_rarQueue.front();
      SendMessage2(e.ue, e.msg3Time, e.msg3RB);
      m_rarQueue.pop();
    }
}


void
EnbNbIoTRandomAccess::SendMessage2(NetworkNode *dest, int msg3time, int msg3RB)
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::SendMessage2("<< dest->GetIDNetworkNode() <<") " << endl;
DEBUG_LOG_END
  RandomAccessResponseIdealControlMessage* msg2 = new RandomAccessResponseIdealControlMessage();
  msg2->SetSourceDevice(m_macEntity->GetDevice());
  msg2->setMsg3Time( msg3time );
  msg2->setMsg3RB( msg3RB );
  msg2->SetDestinationDevice(dest);

DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
  cout << "RANDOM_ACCESS SEND_MSG2 UE " << dest->GetIDNetworkNode()
       << " T " << Simulator::Init()->Now()
       << endl;
DEBUG_LOG_END

  m_macEntity->GetDevice()->GetPhy()->SendIdealControlMessage(msg2);
}


void
EnbNbIoTRandomAccess::ReceiveMessage3(RandomAccessConnectionRequestIdealControlMessage* msg3)
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::ReceiveMessage3() " << endl;
DEBUG_LOG_END
  NetworkNode *src = msg3->GetSourceDevice();
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
  cout << "RANDOM_ACCESS RECEIVE_MSG3 UE " << src->GetIDNetworkNode()
       << " T " << Simulator::Init()->Now()
       << endl;
DEBUG_LOG_END

  Simulator::Init()->Schedule(0.001, &EnbNbIoTRandomAccess::SendMessage4, this,src);
}


void
EnbNbIoTRandomAccess::SendMessage4(NetworkNode *dest)
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::SendMessage4(dest) " << endl;
DEBUG_LOG_END
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
  cout << "RANDOM_ACCESS SEND_MSG4 UE " << dest->GetIDNetworkNode()
       << " T " << Simulator::Init()->Now()
       << endl;
DEBUG_LOG_END

  ENodeB* enb = (ENodeB*)m_macEntity->GetDevice();
  RandomAccessConnectionResolutionIdealControlMessage *msg4;
  msg4 = new RandomAccessConnectionResolutionIdealControlMessage();
  msg4-> SetSourceDevice(enb);
  msg4-> SetDestinationDevice(dest);

  m_macEntity->GetDevice()->GetPhy()->SendIdealControlMessage(msg4);
}


bool
EnbNbIoTRandomAccess::isRachOpportunity()
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::isRachOpportunity() " << endl;
DEBUG_LOG_END
  int currentTTI = FrameManager::Init()->GetTTICounter();
  if (m_tonesReservedForRach.elementsExist(currentTTI))
    {
      return true;
    }
  else
    {
      return false;
    }
}

vector<int>
EnbNbIoTRandomAccess::GetRachReservedSubChannels()
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::GetRachReservedSubChannels() " << endl;
DEBUG_LOG_END
  int currentTTI = FrameManager::Init()->GetTTICounter();
  return m_tonesReservedForRach.getElements(currentTTI);
}


vector<int>
EnbNbIoTRandomAccess::GetCcchReservedSubChannels()
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::GetCcchReservedSubChannels() " << endl;
DEBUG_LOG_END
  int currentTTI = FrameManager::Init()->GetTTICounter();
  return m_RBsReservedForCCCH.getElements(currentTTI);
}


vector<int>
EnbNbIoTRandomAccess::GetReservedSubChannels()
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::GetReservedSubChannels() " << endl;
DEBUG_LOG_END
  int currentTTI = FrameManager::Init()->GetTTICounter();
  vector<int> RachRBs = m_tonesReservedForRach.getElements(currentTTI);
  vector<int> CcchRBs = m_RBsReservedForCCCH.getElements(currentTTI);
  vector<int> result;
  result.reserve( RachRBs.size() + CcchRBs.size() );
  result.insert( result.end(), RachRBs.begin(), RachRBs.end() );
  result.insert( result.end(), CcchRBs.begin(), CcchRBs.end() );
  return result;
}


void
EnbNbIoTRandomAccess::setNbRARs(int n)
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::setNbRARs("<<n<< ") " << endl;
DEBUG_LOG_END
  m_nbRARs = n;
}

int
EnbNbIoTRandomAccess::getNbRARs(void)
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS_NB)
  cout << "EnbNbIoTRandomAccess::getNbRARs() " << endl;
DEBUG_LOG_END
  return m_nbRARs;
}
