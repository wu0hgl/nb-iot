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


#ifndef ENODEB_H_
#define ENODEB_H_

#include "NetworkNode.h"
#include <memory>
#include <armadillo>

class UserEquipment;
class Gateway;

class PacketScheduler;
class DownlinkPacketScheduler;
class UplinkPacketScheduler;

class ENodeB : public NetworkNode
{
public:
  struct UserEquipmentRecord
  {
    UserEquipmentRecord ();
    virtual ~UserEquipmentRecord ();
    UserEquipmentRecord (UserEquipment *UE);

    UserEquipment *m_UE;
    void SetUE (UserEquipment *UE);
    UserEquipment* GetUE (void) const;

    bool m_cqiAvailable;
    bool CqiAvailable(void);

    vector<int> m_cqiFeedback;
    void SetCQI (vector<int> cqi);
    vector<int> GetCQI (void) const;

    int m_riFeedback;
    void SetRI(int ri);
    int GetRI (void) const;

    vector< shared_ptr<arma::cx_fmat> > m_channelMatrix;
    void SetChannelMatrix(vector< shared_ptr<arma::cx_fmat> > channelMatrix);
    vector< shared_ptr<arma::cx_fmat> > GetChannelMatrix(void);

    vector< vector<int> > m_pmiFeedback;
    void SetPMI (vector< vector<int> > pmi);
    vector< vector<int> > GetPMI (void) const;

    int m_schedulingRequest; // in bytes
    void SetSchedulingRequest (int r);
    int GetSchedulingRequest (void);

    int m_averageSchedulingGrants; // in bytes
    void UpdateSchedulingGrants (int b);
    int GetSchedulingGrants (void);

    int m_ulMcs;
    void SetUlMcs (int mcs);
    int GetUlMcs (void);

    vector<double> m_uplinkChannelStatusIndicator;
    void SetUplinkChannelStatusIndicator (vector<double> vet);
    vector<double> GetUplinkChannelStatusIndicator (void) const;

    int m_DlTxMode;
    void SetDlTxMode(int txMode);
    int GetDlTxMode();

    HarqManager* m_harqManager;
    void SetHarqManager (HarqManager* harqManager);
    HarqManager* GetHarqManager (void);
  };

  typedef vector<UserEquipmentRecord*> UserEquipmentRecords;

  enum DLSchedulerType
  {
    DLScheduler_TYPE_MAXIMUM_THROUGHPUT,
    DLScheduler_TYPE_PROPORTIONAL_FAIR,
    DLScheduler_TYPE_FLS,
    DLScheduler_TYPE_MLWDF,
    DLScheduler_TYPE_EXP,
    DLScheduler_LOG_RULE,
    DLScheduler_EXP_RULE,
    DLScheduler_TYPE_ROUND_ROBIN
  };
  enum ULSchedulerType
  {
    ULScheduler_TYPE_MAXIMUM_THROUGHPUT,
    ULScheduler_TYPE_FME,
    ULScheduler_TYPE_ROUNDROBIN,
    ULScheduler_TYPE_NB_IOT_FIFO,
    ULScheduler_TYPE_NB_IOT_ROUNDROBIN
  };

  ENodeB () = default;
  ENodeB (int idElement, Cell *cell);
  ENodeB (int idElement, Cell *cell, double posx, double posy);
  ENodeB (int idElement, Cell *cell, double posx, double posy, double posz);

  virtual ~ENodeB();

  void SetRandomAccessType(EnbRandomAccess::RandomAccessType type);
  void RegisterUserEquipment (UserEquipment *UE);
  void DeleteUserEquipment (UserEquipment *UE);
  int GetNbOfUserEquipmentRecords (void);
  void CreateUserEquipmentRecords (void);
  void DeleteUserEquipmentRecords (void);
  UserEquipmentRecords* GetUserEquipmentRecords (void);
  UserEquipmentRecord* GetUserEquipmentRecord (int idUE);
  EnbMacEntity* GetMacEntity(void) const;

  void SetDLScheduler (DLSchedulerType type);
  DownlinkPacketScheduler* GetDLScheduler (void) const;
  void SetULScheduler (ULSchedulerType type);
  UplinkPacketScheduler* GetULScheduler (void) const;

  void ResourceBlocksAllocation ();
  void UplinkResourceBlockAllocation ();
  void DownlinkResourceBlockAllocation ();

  //Debug
  void Print (void);

private:
  UserEquipmentRecords *m_userEquipmentRecords;
};

#endif /* ENODEB_H_ */
