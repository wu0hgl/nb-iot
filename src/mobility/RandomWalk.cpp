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



#include "RandomWalk.h"
#include "../componentManagers/NetworkManager.h"
#include "../device/Gateway.h"
#include "../device/ENodeB.h"
#include "../device/UserEquipment.h"
#include "../load-parameters.h"


RandomWalk::RandomWalk()
{
  SetMobilityModel(Mobility::RANDOM_WALK);
  SetSpeed (0);
  SetSpeedDirection (0.0);
  SetPositionLastUpdate (0.0);
  SetHandover (false);
  SetLastHandoverTime (0.0);
  SetAbsolutePosition (nullptr);
  m_interval = 0.;
  m_lastTimeDirectionChange = 0.;
}

RandomWalk::~RandomWalk()
{
  DeleteAbsolutePosition ();
}

void
RandomWalk::UpdatePosition(double time)
{
DEBUG_LOG_START_1(LTE_SIM_MOBILITY_DEBUG)
  cout << "\t START MOBILITY MODEL for "<< GetDevice ()->GetIDNetworkNode() << endl;
DEBUG_LOG_END

  if (GetSpeed () == 0)
    {
DEBUG_LOG_START_1(LTE_SIM_MOBILITY_DEBUG)
      cout << "\t\t speed = 0 --> position has not been updated!"<< endl;
DEBUG_LOG_END
      return;
    }

  double timeInterval = time - GetPositionLastUpdate ();

  UserEquipment *thisNode = (UserEquipment*)GetDevice ();
  Cell *thisCell = thisNode->GetCell ();
  NetworkNode *targetNode = thisNode->GetTargetNode ();

DEBUG_LOG_START_1(LTE_SIM_MOBILITY_DEBUG)
  cout << "MOBILITY_DEBUG: User ID: " << GetDevice ()->GetIDNetworkNode()
            << "\n\t Cell ID " <<
            NetworkManager::Init()->GetCellIDFromPosition (GetAbsolutePosition()->GetCoordinateX(),
                GetAbsolutePosition()->GetCoordinateY())
            << "\n\t Initial Position (X): " << GetAbsolutePosition()->GetCoordinateX()
            << "\n\t Initial Position (Y): " << GetAbsolutePosition()->GetCoordinateY()
            << "\n\t Speed: " << GetSpeed()
            << "\n\t Speed Direction: " << GetSpeedDirection()
            << "\n\t Time Last Update: " << GetPositionLastUpdate()
            << "\n\t Time Interval: " << timeInterval
            << endl;
DEBUG_LOG_END
DEBUG_LOG_START_1(LTE_SIM_MOBILITY_DEBUG_TAB)
  cout << GetDevice ()->GetIDNetworkNode()  << ""
            << GetAbsolutePosition()->GetCoordinateX() << " "
            << GetAbsolutePosition()->GetCoordinateY() << " "
            << GetSpeed() << " "
            << GetSpeedDirection() << " "
            << GetPositionLastUpdate() << " "
            << timeInterval << " "
            << endl;
DEBUG_LOG_END

  double shift = timeInterval * (GetSpeed()*(1000.0/3600.0));

  double shift_y =
    shift * sin (GetSpeedDirection());
  double shift_x =
    shift * cos (GetSpeedDirection());

  CartesianCoordinates *newPosition =
    new CartesianCoordinates(GetAbsolutePosition()->GetCoordinateX()+shift_x,
                             GetAbsolutePosition()->GetCoordinateY()+shift_y,
                             GetAbsolutePosition()->GetCoordinateZ());
  newPosition->SetFloorHeight( GetAbsolutePosition()->GetFloorHeight() );

  CartesianCoordinates *ENodeBPosition = targetNode->GetMobilityModel ()->GetAbsolutePosition ();

  const double pi = M_PI;
  double azimut = newPosition->GetPolarAzimut (ENodeBPosition);
  double newDistanceFromTheENodeB = newPosition->GetDistance (ENodeBPosition);

  if (newDistanceFromTheENodeB <= (thisCell->GetMinDistance() * 1000))
    {
      CartesianCoordinates *Correction =
        new CartesianCoordinates (((thisCell->GetMinDistance()*1000) - newDistanceFromTheENodeB) * cos(azimut),
                                  ((thisCell->GetMinDistance()*1000) - newDistanceFromTheENodeB) * sin(azimut));

      newPosition->SetCoordinateX(newPosition->GetCoordinateX() + Correction->GetCoordinateX());
      newPosition->SetCoordinateY(newPosition->GetCoordinateY() + Correction->GetCoordinateY());

      if ((azimut > GetSpeedDirection ()-pi/2) && (azimut < GetSpeedDirection ()+pi/2))
        {
          double speedDirection = (double)(rand() %360) * ((2*M_PI)/360);
          SetSpeedDirection(speedDirection);
        }

      delete Correction;
    }
  else if (newDistanceFromTheENodeB > (thisCell->GetRadius()*1000))
    {
      if (GetHandover()== false)
        {
          //the UE must remain into the same cell
          CartesianCoordinates *Correction =
            new CartesianCoordinates ((newDistanceFromTheENodeB - (thisCell->GetRadius()*1000)) * cos(azimut),
                                      (newDistanceFromTheENodeB - (thisCell->GetRadius()*1000)) * sin(azimut));

          newPosition->SetCoordinateX(newPosition->GetCoordinateX() - Correction->GetCoordinateX());
          newPosition->SetCoordinateY(newPosition->GetCoordinateY() - Correction->GetCoordinateY());

          double speedDirection = (double)(rand() %360) * ((2*M_PI)/360);
          SetSpeedDirection(speedDirection);

          delete Correction;
        }
      else if (newPosition->GetDistance(0.0, 0.0) >= GetTopologyBorder ())
        {
          CartesianCoordinates *Correction =
            new CartesianCoordinates ((newDistanceFromTheENodeB - (thisCell->GetRadius()*1000)) * cos(azimut),
                                      (newDistanceFromTheENodeB - (thisCell->GetRadius()*1000)) * sin(azimut));

          newPosition->SetCoordinateX(newPosition->GetCoordinateX() - Correction->GetCoordinateX());
          newPosition->SetCoordinateY(newPosition->GetCoordinateY() - Correction->GetCoordinateY());

          double speedDirection = (double)(rand() %360) * ((2*M_PI)/360);
          SetSpeedDirection(speedDirection);

          delete Correction;
        }
    }

  SetAbsolutePosition(newPosition);
  SetPositionLastUpdate (time);

DEBUG_LOG_START_1(LTE_SIM_MOBILITY_DEBUG)
  cout << "\n\t Final Position (X): " << GetAbsolutePosition()->GetCoordinateX()
            << "\n\t Final Position (Y): " << GetAbsolutePosition()->GetCoordinateY()
            << endl;
DEBUG_LOG_END
DEBUG_LOG_START_1(LTE_SIM_MOBILITY_DEBUG_TAB)
  cout << GetAbsolutePosition()->GetCoordinateX() << " "
            << GetAbsolutePosition()->GetCoordinateY()
            << endl;
DEBUG_LOG_END

  delete newPosition;

  if (time - m_lastTimeDirectionChange >= m_interval)
    {
      double speedDirection = (double)(rand() %360) * ((2*M_PI)/360);
      SetSpeedDirection(speedDirection);

      double averageDistance;
      double a;
      double b;

      if (GetSpeed()==3)
        {
          averageDistance = 200;
        }
      else if (GetSpeed() ==30)
        {
          averageDistance = 400;
        }
      else if (GetSpeed()==120)
        {
          averageDistance = 1000;
        }
      else
        {
          averageDistance = 200;
        }

      a = averageDistance - 100;
      b = averageDistance + 100;

      double distance = (double) (rand() %1001);
      distance = distance/1000;
      distance = a + (distance * (b-a)); //m

      m_interval = distance / (GetSpeed()/3.6); //s
      m_lastTimeDirectionChange = time;
    }
}
