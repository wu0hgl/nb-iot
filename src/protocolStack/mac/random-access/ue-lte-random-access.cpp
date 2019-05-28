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

#include "ue-lte-random-access.h"
#include "../../../componentManagers/FrameManager.h"
#include "../../../device/UserEquipment.h"


UeLteRandomAccess::UeLteRandomAccess()
{
  m_type = RA_TYPE_LTE;
  m_macEntity = NULL;
  m_backoffIndicator = 5;
}


UeLteRandomAccess::UeLteRandomAccess(MacEntity* mac) : UeLteRandomAccess()
{
  m_macEntity = mac;
}


UeLteRandomAccess::~UeLteRandomAccess()
{
}


void
UeLteRandomAccess::StartRaProcedure()
{

  if ((m_macEntity->GetDevice()->GetNodeState()!= NetworkNode::STATE_ACTIVE))
    {
      if (m_RaProcedureActive == false)
        {
          m_RaProcedureActive = true;

DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
          cout << "RANDOM_ACCESS START UE " << m_macEntity ->GetDevice()->GetIDNetworkNode() << " T " << Simulator::Init()->Now() << endl;
DEBUG_LOG_END

          UeRandomAccess::SetRAProcedureStartTime(Simulator::Init()->Now());
          Simulator::Init()->Schedule(m_RAProcedureTimeout, &UeLteRandomAccess::ReStartRaProcedure, this);

          SendMessage1();
        }
    }

  else
    {
      UserEquipment* ue = (UserEquipment*)m_macEntity->GetDevice();
      ue -> SetLastActivity();
    }

}


void
UeLteRandomAccess::ReStartRaProcedure()
{
  if (m_RaProcedureActive == true)
    {
      m_RaProcedureActive = false;
      m_nbFailedAttempts++;
      if (m_nbFailedAttempts < m_maxFailedAttempts)
        {
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
          cout << "RANDOM_ACCESS RESTART UE " << m_macEntity ->GetDevice()->GetIDNetworkNode() << " TRY " << m_nbFailedAttempts << " T " << Simulator::Init()->Now() << endl;
DEBUG_LOG_END
          int maxWaitingTime;
          if (m_backoffIndicator == 0)
            {
              maxWaitingTime = 0;
            }
          else if (m_backoffIndicator <= 12)
            {
              maxWaitingTime = pow(2,m_backoffIndicator+7);
            }
          else
            {
              cout << "Error: invalid backoff indicator " << m_backoffIndicator << endl;
              exit(1);
            }
          int waitingTime = rand() % maxWaitingTime;
          Simulator::Init()->Schedule(0.001*waitingTime,&UeLteRandomAccess::StartRaProcedure,this);
        }
      else
        {
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
          cout << "RANDOM_ACCESS FAIL UE " << m_macEntity ->GetDevice()->GetIDNetworkNode() << " T " << Simulator::Init()->Now() << endl;
DEBUG_LOG_END

          m_nbFailedAttempts = 0;
        }
    }
}


void
UeLteRandomAccess::SendMessage1()
{
  if( isRachOpportunity() )
    {
      RandomAccessPreambleIdealControlMessage* msg1 = new RandomAccessPreambleIdealControlMessage();
      msg1-> SetSourceDevice(m_macEntity->GetDevice());

      UserEquipment* ue = (UserEquipment*)m_macEntity->GetDevice();
      msg1->SetDestinationDevice (ue->GetTargetNode());

      m_macEntity->GetDevice()->GetPhy()->SendIdealControlMessage(msg1);
    }
  else
    {
      Simulator::Init()->Schedule(0.001,&UeLteRandomAccess::SendMessage1,this);
    }
}


void
UeLteRandomAccess::ReceiveMessage2(int msg3time)
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
  cout << "RANDOM_ACCESS RECEIVE_MSG2 UE " << m_macEntity ->GetDevice()->GetIDNetworkNode() << " T " << Simulator::Init()->Now() << endl;
DEBUG_LOG_END

  double delay = msg3time/1000.0 - Simulator::Init()->Now();

  Simulator::Init()->Schedule(delay, &UeLteRandomAccess::SendMessage3, this);
}


void
UeLteRandomAccess::SendMessage3()
{
DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
  cout << "RANDOM_ACCESS SEND_MSG3 UE " << m_macEntity ->GetDevice()->GetIDNetworkNode() << " T " << Simulator::Init()->Now() << endl;
DEBUG_LOG_END

  m_msg3 = new RandomAccessConnectionRequestIdealControlMessage();
  m_msg3->SetSourceDevice(m_macEntity->GetDevice());


  UserEquipment* ue = (UserEquipment*)m_macEntity->GetDevice();
  m_msg3->SetDestinationDevice (ue->GetTargetNode());
  m_macEntity->GetDevice()->GetPhy()->SendIdealControlMessage(m_msg3);
}


void
UeLteRandomAccess::ReceiveMessage4()
{

DEBUG_LOG_START_1(LTE_SIM_TEST_RANDOM_ACCESS)
  cout << "RANDOM_ACCESS RECEIVE_MSG4 UE " << m_macEntity ->GetDevice()->GetIDNetworkNode() << " T " << Simulator::Init()->Now() << endl;
DEBUG_LOG_END

  UeMacEntity* mac = (UeMacEntity*)m_macEntity;
  mac-> GetDevice()->MakeActive();
  mac->SendSchedulingRequest();

  UeRandomAccess::SetRaProcedureActive(false);
  m_nbFailedAttempts = 0;
}
