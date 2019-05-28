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
 * Author: Cosimo Damiano Di Pace <cd.dipace@gmail.com>
 * Author: Alessandro Grassi <alessandro.grassi@poliba.it>
 */

#include "enb-lte-random-access.h"
#include "../../../componentManagers/FrameManager.h"
#include "../../../core/eventScheduler/simulator.h"
#include "../../../device/ENodeB.h"
#include "ue-lte-random-access.h"
#include "../enb-mac-entity.h"
#include "../../../core/idealMessages/ideal-control-messages.h"
#include "../../../device/NetworkNode.h"
#include "../../../phy/lte-phy.h"
#include "../../../core/spectrum/bandwidth-manager.h"


EnbLteRandomAccess::EnbLteRandomAccess()
{
  m_PreambleFormat = Preamble_FORMAT_0;
  m_type = RA_TYPE_LTE;
  m_lastRachOpportunity = 0;
  m_ripInt = 5; // repetition interval in TTIs
  m_PreambleDuration = 1; // TTIs
  m_macEntity = NULL;
  m_maxCCCHUsage = 0.75;
  m_responseWindowDelay = 3; // TTIs
  m_responseWindowDuration = 5; // TTIs

  m_RachReservation = Simulator::Init()->Schedule(0.001, &EnbLteRandomAccess::SetRachReservedSubChannels, this);
  m_RarScheduling = Simulator::Init()->Schedule(0.001, &EnbLteRandomAccess::CheckCollisions, this);
}


EnbLteRandomAccess::EnbLteRandomAccess(MacEntity* mac) : EnbLteRandomAccess()
{
  m_macEntity = mac;
}


EnbLteRandomAccess::~EnbLteRandomAccess()
{
  m_RachReservation->MarkDeleted();
  m_RarScheduling->MarkDeleted();
}


void
EnbLteRandomAccess::SetRachReservedSubChannels()
{
  int currentTTI = FrameManager::Init()->GetTTICounter();

  int predictionWindow = m_PreambleDuration + m_responseWindowDelay + 2*m_responseWindowDuration;

  m_RBsReservedForRach.deleteOldElements(currentTTI);

  for (int tti=currentTTI; tti<=currentTTI+predictionWindow; tti++)
    {
      if (tti % m_ripInt < (m_PreambleDuration))
        {
          if ( ! m_RBsReservedForRach.elementsExist(tti) )
            {
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
              cout << "RANDOM_ACCESS RACH session planned at TTI " << tti << endl;
DEBUG_LOG_END
              for(int i=0; i<6; i++)
                {
                  m_RBsReservedForRach.addElement(tti,i);
                }
            }
        }
    }

  if (m_RBsReservedForRach.elementsExist(currentTTI) )
    {
      m_RachReservation = Simulator::Init()->Schedule(m_ripInt/1000.0, &EnbLteRandomAccess::SetRachReservedSubChannels,this);
    }
  else
    {
      m_RachReservation = Simulator::Init()->Schedule(0.001, &EnbLteRandomAccess::SetRachReservedSubChannels,this);
    }
}


vector<int>
EnbLteRandomAccess::GetRachReservedSubChannels()
{
  int currentTTI = FrameManager::Init()->GetTTICounter();
  return m_RBsReservedForRach.getElements(currentTTI);
}


vector<int>
EnbLteRandomAccess::GetCcchReservedSubChannels()
{
  int currentTTI = FrameManager::Init()->GetTTICounter();
  return m_RBsReservedForCCCH.getElements(currentTTI);
}


vector<int>
EnbLteRandomAccess::GetReservedSubChannels()
{
  int currentTTI = FrameManager::Init()->GetTTICounter();
  vector<int> RachRBs = m_RBsReservedForRach.getElements(currentTTI);
  vector<int> CcchRBs = m_RBsReservedForCCCH.getElements(currentTTI);
  vector<int> result;
  result.reserve( RachRBs.size() + CcchRBs.size() );
  result.insert( result.end(), RachRBs.begin(), RachRBs.end() );
  result.insert( result.end(), CcchRBs.begin(), CcchRBs.end() );
  return result;
}

void
EnbLteRandomAccess::ReceiveMessage1( RandomAccessPreambleIdealControlMessage *msg)
{

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
EnbLteRandomAccess::CheckCollisions()
{
  int currentTTI = FrameManager::Init()->GetTTICounter();
  int nbOfRBs = m_macEntity->GetDevice ()->GetPhy ()->GetBandwidthManager ()->GetUlSubChannels ().size ();

  int verifyTime = currentTTI - m_responseWindowDelay;

  map <int, rachSession>::iterator it;

  it = m_savedRachSessions.find(verifyTime);

  if(it != m_savedRachSessions.end())
   {
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
      cout << "RANDOM_ACCESS RACH session found at TTI " << verifyTime << endl;
DEBUG_LOG_END
      queue< pair<int,int> > RBsForCCCH;
      for (int targetTime=currentTTI+1; targetTime<=currentTTI+m_responseWindowDuration;targetTime++)
        {
          vector<int> availableRBs;
          for (int rb=0; rb<nbOfRBs; rb++)
            {
              if ( m_RBsReservedForRach.isAllocated(targetTime,rb)==false )
                {
                  if (m_RBsReservedForCCCH.isAllocated(targetTime,rb)==false)
                    {
                      availableRBs.push_back(rb);
                    }
                }
            }
          for (unsigned int i=0; i<availableRBs.size(); i++)
            {
              if (availableRBs.at(i)<round(nbOfRBs*m_maxCCCHUsage))
                {
                  RBsForCCCH.push( std::pair<int,int>(targetTime,availableRBs.at(i)) );
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
                  cout << "RANDOM_ACCESS RB Available for CCCH: " << availableRBs.at(i) << " at TTI " << targetTime << endl;
DEBUG_LOG_END
                }
            }
        }

      vector<int> collidedPreambles;

      rachSession messages = (*it).second;
      for (auto k : messages)
        {
          bool collision = false;

          for (auto j : messages)
            {
              if (k.preamble == j.preamble && k.ue != j.ue)
                {
                  collision = true;

DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
                  cout << "RANDOM_ACCESS COLLISION UE " << k.ue->GetIDNetworkNode() << " PREAMBLE "
                       << k.preamble << " TIME " << verifyTime << endl;
DEBUG_LOG_END
                }
            }
          if (collision == false)
            {
              if ( ! RBsForCCCH.empty() )
                {
                  rarElement e;
                  pair<int,int> CCCHResource = RBsForCCCH.front();
                  e.msg1Time = verifyTime;
                  e.msg3Time = CCCHResource.first;
                  e.msg3RB = CCCHResource.second;
                  e.ue = k.ue;
                  m_RBsReservedForCCCH.addElement(e.msg3Time, e.msg3RB);
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
                  cout << "RANDOM_ACCESS Reserving RB for CCCH: " << CCCHResource.second << " at TTI " << CCCHResource.first << endl;
DEBUG_LOG_END
                  m_rarQueue.push(e);
                  RBsForCCCH.pop();
                }
              else
                {
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
                  cout << "RANDOM_ACCESS SEND_MSG2_FAILED UE " << k.ue->GetIDNetworkNode() << " T " << Simulator::Init()->Now() << endl;
DEBUG_LOG_END
                }
            }
          else
            {
              if (find(collidedPreambles.begin(),collidedPreambles.end(),k.preamble)==collidedPreambles.end())
                {
                  collidedPreambles.push_back(k.preamble);
                  if ( ! RBsForCCCH.empty() )
                    {
                      RBsForCCCH.pop();
                    }
                }
            }
        }
      m_savedRachSessions.erase(verifyTime);
    }

  if ( !m_rarQueue.empty() )
      {
        SendResponses();
      }

    m_RBsReservedForCCCH.deleteOldElements(currentTTI);

    m_RarScheduling = Simulator::Init()->Schedule(0.001, &EnbLteRandomAccess::CheckCollisions, this);
  }


  void
  EnbLteRandomAccess::SendResponses()
  {
    while( ! m_rarQueue.empty())
      {
        rarElement e = m_rarQueue.front();
        SendMessage2(e.ue, e.msg3Time, e.msg3RB);
        m_rarQueue.pop();
      }
  }


  void
  EnbLteRandomAccess::SendMessage2(NetworkNode* dest, int msg3time, int msg3RB)
  {

    RandomAccessResponseIdealControlMessage* msg2 = new RandomAccessResponseIdealControlMessage();
    msg2->SetSourceDevice(m_macEntity->GetDevice());
    msg2->setMsg3Time( msg3time );
    msg2->setMsg3RB( msg3RB );

  DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
    cout << "RANDOM_ACCESS SEND_MSG2 UE " << dest->GetIDNetworkNode() << " T " << Simulator::Init()->Now() << endl;
  DEBUG_LOG_END

    msg2->SetDestinationDevice(dest);
    m_macEntity->GetDevice()->GetPhy()->SendIdealControlMessage(msg2);

  }

  void
  EnbLteRandomAccess::ReceiveMessage3(RandomAccessConnectionRequestIdealControlMessage* msg)
  {
    NetworkNode *src = msg->GetSourceDevice();
  DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
    cout << "RANDOM_ACCESS RECEIVE_MSG3 UE " << src->GetIDNetworkNode() << " T " << Simulator::Init()->Now() << endl;
  DEBUG_LOG_END

    Simulator::Init()->Schedule(0.001, &EnbLteRandomAccess::SendMessage4, this,src);
  }


  void
  EnbLteRandomAccess::SendMessage4(NetworkNode *dest)
  {

  DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
    cout << "RANDOM_ACCESS SEND_MSG4 UE " << dest->GetIDNetworkNode() << " T " << Simulator::Init()->Now() << endl;
  DEBUG_LOG_END

    ENodeB* enb = (ENodeB*)m_macEntity->GetDevice();
    RandomAccessConnectionResolutionIdealControlMessage *msg4;
    msg4 = new RandomAccessConnectionResolutionIdealControlMessage();
    msg4-> SetSourceDevice(enb);
    msg4-> SetDestinationDevice(dest);

    m_macEntity->GetDevice()->GetPhy()->SendIdealControlMessage(msg4);
  }

  bool
  EnbLteRandomAccess::isRachOpportunity()
  {
    int currentTTI = FrameManager::Init()->GetTTICounter();
    if (m_RBsReservedForRach.elementsExist(currentTTI))
      {
        return true;
      }
    else
      {
        return false;
      }
  }


