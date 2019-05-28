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


#ifndef NetworkManager_H_
#define NetworkManager_H_

#include <stdint.h>
#include <vector>
#include "../load-parameters.h"
#include "../networkTopology/Cell.h"

class NetworkNode;
class Cell;
class Femtocell;
class ENodeB;
class HeNodeB;
class UserEquipment;
class Gateway;
class Mobility;
class CartesianCoordinates;
class BandwidthManager;
class LteChannel;
class Building;
class Street;

class NetworkManager
{

public:
  enum WrapAroundType
  {
    WRAPAROUND_NONE,
    WRAPAROUND_X_AXIS,
    WRAPAROUND_Y_AXIS, // not implemented
    WRAPAROUND_X_Y_AXIS // not implemented
  };

private:
  vector<Cell*> *m_cellContainer;
  vector<Femtocell*> *m_femtoCellContainer;
  vector<ENodeB*> *m_eNodeBContainer;
  vector<HeNodeB*> *m_homeENodeBContainer;
  vector<UserEquipment*> *m_userEquipmentContainer;
  vector<Gateway*> *m_gatewayContainer;
  vector<Building*> *m_buildingContainer;
  WrapAroundType m_WrapAroundType;
  double m_WrapAroundDistanceX;
  double m_WrapAroundDistanceY;

  NetworkManager();
  static NetworkManager *ptr;


public:
  virtual ~NetworkManager();

  static NetworkManager*
  Init (void)
  {
    if (ptr==nullptr)
      {
        ptr = new NetworkManager;
      }
    return ptr;
  }

  /*
   * Get a Network Element Container
   */
  vector<Cell*>*
  GetCellContainer (void);
  vector<Femtocell*>*
  GetFemtoCellContainer (void);
  vector<ENodeB*>*
  GetENodeBContainer (void);
  vector<HeNodeB*>*
  GetHomeENodeBContainer (void);
  vector<UserEquipment*>*
  GetUserEquipmentContainer (void);
  vector<Gateway*>*
  GetGatewayContainer (void);
  vector<Building*>*
  GetBuildingContainer (void);

  int
  GetNbCell(void);

  /*
   * Get Network Elements
   */
  Cell*
  GetCellByID (int idCell);
  Femtocell*
  GetFemtoCellByID (int idFemtoCell);
  ENodeB*
  GetENodeBByID (int idENodeB);
  ENodeB*
  GetENodeBByCellID (int idCell);
  UserEquipment*
  GetUserEquipmentByID (int idUE);
  Gateway*
  GetGatewayByID (int idGW);
  Building*
  GetBuildingByID (int idBuilding);
  Building*
  GetBuildingByFemtoCellID (int idFemtoCell);

  vector<UserEquipment*>*
  GetRegisteredUEToENodeB (int idENB);

  NetworkNode*
  GetNetworkNodeByID (int id);

  WrapAroundType
  GetWrapAroundType(void);
  void
  SetWrapAroundType(WrapAroundType type);
  void
  SetWrapAroundDistanceX(double distance);
  double
  GetWrapAroundDistanceX(void);
  void
  SetWrapAroundDistanceY(double distance);
  double
  GetWrapAroundDistanceY(void);


  /*
   * Get Position-related Info for Network Elements
   */
  vector<int>
  GetCellIDFromPosition (CartesianCoordinates *position);
  int
  GetCellIDFromPosition (double posX, double posY);



  bool
  CheckIndoorUsers (UserEquipment *ue);
  int
  GetBuildingIDForUE (UserEquipment *ue);

  Cell*
  GetBelongingCellFromPosition(UserEquipment* ue);
  void
  SelectTargetNode (UserEquipment* ue);

  bool
  CheckHandoverNeed (int idUserEquipment);
  bool
  CheckHandoverPermissions (ENodeB* target, UserEquipment* user);
  void
  HandoverProcedure (double time, UserEquipment* ue, ENodeB* oldTarget, ENodeB* newTarget);
  void
  TransferBearerInfo (UserEquipment* ue, ENodeB* target);

  void
  TransferPacketsThroughX2 (int bearerID,
                            int targetENB);

  vector <BandwidthManager*>
  RunFrequencyReuse (int nodes, int cluster, double bandwidth);

  void
  PrintUEsForEachCell (void);




  Cell* CreateCell (int idCell, double radius, double minDistance, double X, double Y);

  ENodeB* CreateEnodeb (int id,
                        Cell* cell,
                        double pos_X, double pos_Y,
                        int numTxAntennas, int numRxAntennas,
                        LteChannel *dlCh, LteChannel *ulCh,
                        BandwidthManager *bm);

  Gateway* CreateGateway (void);

  UserEquipment* CreateUserEquipment (int id,
                                      double pos_X, double pos_Y, double speed, double speedDirection, int numTxAntennas, int numRxAntennas,
                                      Cell* cell, ENodeB* enb);

  void CreateStreet (int idStreet, CartesianCoordinates* center, double theta,
                     int buildingtype, int nbBuildings, double street_width, double building_distance,
                     int nbFemtoInBuilding, double apartment_side, int firstFemtoIDInStreet);
  void CreateBuildingForFemtocells (int idBuilding, int buildingType, double apartmentSize, int nbFloors, double pos_X, double pos_Y, int firsFemtoCellID, int nbOfFemtocells);
  Femtocell* CreateFemtoCell(int idCell, double side, double X, double Y);
  double* CheckBuildingPosition (int buildingType, double pos_X, double pos_Y, double side_X, double side_Y);

//Debug
  enum m_debugInfo
  {
    ALL_NETWORK,
    ONLY_CELL_DETAILS,
    ONLY_ENODEB_DETAILS,
    ONLY_USER_EQUIPMENT_DETAILS
  };
//  void
//  Print (void);
  void
  Print (NetworkManager::m_debugInfo info);

  void
  PrintFrequencyMask (void);

  void
  PrintUserPosition (void);
};


#endif /* NetworkManager_H_ */



