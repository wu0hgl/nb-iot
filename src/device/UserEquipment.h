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



#ifndef USEREQUIPMENT_H_
#define USEREQUIPMENT_H_

#include "NetworkNode.h"
#include "ENodeB.h"
#include "../core/eventScheduler/event.h"
#include "../protocolStack/mac/ue-mac-entity.h"

class Gateway;
class CqiManager;


class UserEquipment : public NetworkNode
{
public:
  UserEquipment () = default;
  UserEquipment (int idElement,
                 double posx, double posy,
                 Cell *cell,
                 ENodeB* target,
                 bool handover, Mobility::MobilityModel model);
  UserEquipment (int idElement,
                 double posx, double posy, int speed, double speedDirection,
                 Cell *cell,
                 ENodeB* target,
                 bool handover, Mobility::MobilityModel model);

  virtual ~UserEquipment();

  void SetTargetNode (ENodeB *n);
  ENodeB* GetTargetNode (void);

  void SetTargetNodeRecord (ENodeB::UserEquipmentRecord *r);
  ENodeB::UserEquipmentRecord* GetTargetNodeRecord (void);
  void UpdateUserPosition (double time);
  UeMacEntity* GetMacEntity(void) const;

  void SetCqiManager (CqiManager *cm);
  CqiManager* GetCqiManager (void);

  void SetTimePositionUpdate (double time);
  double GetTimePositionUpdate (void);

  void
  SetIndoorFlag ( bool flag );
  virtual bool
  IsIndoor (void);

  void SetLastActivity();

  void SetActivityTimeout(double timeout);
  double GetActivityTimeout();
  void SetRandomAccessType(UeRandomAccess::RandomAccessType type);
  //Debug
  void Print (void);

private:
  ENodeB* m_targetNode;
  ENodeB::UserEquipmentRecord* m_targetNodeRecord;
  CqiManager *m_cqiManager;

  bool m_isIndoor;

  double m_timePositionUpdate;
  double m_activityTimeout;
  shared_ptr<Event> m_activityTimeoutEvent;
};

#endif /* USEREQUIPMENT_H_ */
