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


#include "position-based-ho-manager.h"
#include "../../../device/NetworkNode.h"
#include "../../../device/UserEquipment.h"
#include "../../../device/ENodeB.h"
#include "../../../device/HeNodeB.h"
#include "../../../componentManagers/NetworkManager.h"

PositionBasedHoManager::PositionBasedHoManager()
{
  m_target = nullptr;
}

bool
PositionBasedHoManager::CheckHandoverNeed (UserEquipment* ue)
{
  ENodeB *targetNode = ue->GetTargetNode ();

  CartesianCoordinates *uePosition = ue->GetMobilityModel ()->GetAbsolutePosition ();
  CartesianCoordinates *targetPosition;

  targetPosition = targetNode->GetMobilityModel ()->GetAbsolutePosition ();
  double targetDistance = uePosition->GetDistance (targetPosition);

  double targetAngularDistance;
  if (targetNode->GetPhy ()->GetAntennaParameters ()->GetType()==LtePhy::AntennaParameters::ANTENNA_TYPE_TRI_SECTOR)
    {
      double sectorBearing = targetNode->GetPhy ()->GetAntennaParameters ()->GetBearing()*M_PI/180.0;
      targetAngularDistance = uePosition->GetPolarAzimut(targetPosition) - sectorBearing;
      if ( targetAngularDistance > M_PI )
        {
          targetAngularDistance -= (2*M_PI);
        }
    }
  else
    {
      targetAngularDistance = 0;
    }
  /*
  if (targetDistance <= (ue->GetCell ()->GetRadius () * 0.8))
    {
    return false;
    }
  */

  for (auto probableNewTargetNode : *NetworkManager::Init ()->GetENodeBContainer ())
    {
      if (probableNewTargetNode->GetIDNetworkNode () != targetNode->GetIDNetworkNode () )
        {
          CartesianCoordinates* enbPosition = probableNewTargetNode->GetMobilityModel ()->GetAbsolutePosition ();
          double distance = enbPosition->GetDistance (uePosition);

          double angularDistance;
          if (probableNewTargetNode->GetPhy ()->GetAntennaParameters ()->GetType()==LtePhy::AntennaParameters::ANTENNA_TYPE_TRI_SECTOR)
            {
              double sectorBearing = probableNewTargetNode->GetPhy ()->GetAntennaParameters ()->GetBearing()*M_PI/180.0;
              angularDistance = uePosition->GetPolarAzimut(enbPosition) - sectorBearing;
              if ( angularDistance > M_PI )
                {
                  angularDistance -= (2*M_PI);
                }
            }
          else
            {
              angularDistance = 0;
            }

          if ( (distance < targetDistance) || (distance == targetDistance && abs(angularDistance) < abs(targetAngularDistance)) )
            {
              if (NetworkManager::Init()->CheckHandoverPermissions(probableNewTargetNode,ue))
                {
                  targetDistance = distance;
                  targetNode = probableNewTargetNode;
                  targetAngularDistance = angularDistance;
                }
            }
        }
    }

  for (auto probableNewTargetNode: *NetworkManager::Init ()->GetHomeENodeBContainer())
    {
      if (probableNewTargetNode->GetIDNetworkNode () != targetNode->GetIDNetworkNode () )
        {
          double distance = probableNewTargetNode->GetMobilityModel ()->
                            GetAbsolutePosition ()->GetDistance (uePosition);

          if (distance < targetDistance)
            {
              if (NetworkManager::Init()->CheckHandoverPermissions(probableNewTargetNode,ue))
                {
                  targetDistance = distance;
                  targetNode = probableNewTargetNode;
                }
            }
        }
    }

  if (ue->GetTargetNode ()->GetIDNetworkNode () != targetNode->GetIDNetworkNode ())
    {
      m_target = targetNode;
      return true;
    }
  else
    {
      return false;
    }
}
