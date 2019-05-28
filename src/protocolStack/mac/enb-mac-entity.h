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


#ifndef ENB_MAC_ENTITY_H
#define ENB_MAC_ENTITY_H

#include <list>
#include "mac-entity.h"
#include "random-access/enb-random-access.h"
#include "../../core/idealMessages/ideal-control-messages.h"

/*
 * This class implements the MAC layer of the eNodeB device
 */

class PacketScheduler;
class DownlinkPacketScheduler;
class UplinkPacketScheduler;
class EnbRandomAccess;
class ENodeB;

class EnbMacEntity : public MacEntity
{
public:

  EnbMacEntity (void);
  virtual ~EnbMacEntity (void);

  ENodeB* GetDevice(void);

  void SetUplinkPacketScheduler (UplinkPacketScheduler* s);
  void SetDownlinkPacketScheduler (DownlinkPacketScheduler* s);
  UplinkPacketScheduler* GetUplinkPacketScheduler (void);
  DownlinkPacketScheduler* GetDownlinkPacketScheduler (void);

  void ReceiveIdealControlMessage (IdealControlMessage* msg);
  void ReceiveCqiIdealControlMessage  (CqiIdealControlMessage* msg);
  void ReceiveRiIdealControlMessage  (RankIndicatorIdealControlMessage* msg);
  void ReceivePmiIdealControlMessage  (PmiIdealControlMessage* msg);
  void ReceiveFullCsiIdealControlMessage  (FullCsiIdealControlMessage* msg);
  void SendPdcchMapIdealControlMessage  (PdcchMapIdealControlMessage* msg);
  void ReceiveSchedulingRequestIdealControlMessage (SchedulingRequestIdealControlMessage *msg);
  void ReceiveHarqAckIdealControlMessage (HarqIdealControlMessage *msg);
  void ReceiveRandomAccessPreambleIdealControlMessage (RandomAccessPreambleIdealControlMessage* msg);
  void ReceiveRandomAccessConnectionRequestIdealControlMessage (RandomAccessConnectionRequestIdealControlMessage* msg);
  EnbRandomAccess* GetRandomAccessManager(void);
  void SetRandomAccessManager(EnbRandomAccess* s);
  void SetRandomAccessType (EnbRandomAccess::RandomAccessType type);

  void SetDefaultDlTxMode (int txmode);
  int GetDefaultDlTxMode(void);

private:

  UplinkPacketScheduler* m_uplinkScheduler;
  DownlinkPacketScheduler* m_downlinkScheduler;
  EnbRandomAccess* m_enbrandomaccess;
  RandomAccessResponseIdealControlMessage* flag;
  int m_defaultDlTxMode;
};


#endif /* ENB_MAC_ENTITY_H */
