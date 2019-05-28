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
 * Author: Alessandro Grassi <alessandro.grassi@poliba.it>
 */

#include "../channel/LteChannel.h"
#include "../phy/enb-lte-phy.h"
#include "../phy/ue-lte-phy.h"
#include "../core/spectrum/bandwidth-manager.h"
#include "../networkTopology/Cell.h"
#include "../protocolStack/packet/packet-burst.h"
#include "../protocolStack/mac/packet-scheduler/downlink-packet-scheduler.h"
#include "../protocolStack/packet/Packet.h"
#include "../protocolStack/rrc/ho/handover-entity.h"
#include "../protocolStack/rrc/ho/power-based-ho-manager.h"
#include "../core/eventScheduler/simulator.h"
#include "../flows/application/InfiniteBuffer.h"
#include "../flows/application/VoIP.h"
#include "../flows/application/CBR.h"
#include "../flows/application/FTP2.h"
#include "../flows/application/TraceBased.h"
#include "../device/IPClassifier/ClassifierParameters.h"
#include "../flows/QoS/QoSParameters.h"
#include "../flows/QoS/QoSForEXP.h"
#include "../flows/QoS/QoSForFLS.h"
#include "../flows/QoS/QoSForM_LWDF.h"
#include "../componentManagers/FrameManager.h"
#include "../utility/RandomVariable.h"
#include "../utility/UsersDistribution.h"
#include "../load-parameters.h"
#include <iostream>
#include <queue>
#include <fstream>
#include <stdlib.h>
#include <cstring>

static void f5g_HighSpeedTrain (int argc, char *argv[])
{
  string environment(argv[2]); // "suburban" or "rural"
  double isd = atof(argv[3]); // km
  int nbUE = atoi(argv[4]);
  int speed = atoi(argv[5]); // km/h
  double duration = atof(argv[6]); // s
  int txMode = atoi(argv[7]);
  int nbTx = atoi(argv[8]);
  int nbMu = atoi(argv[9]);
  int nbRx = atoi(argv[10]);
  int sched_type = atoi(argv[11]);
  int use_srta_pi = atoi(argv[12]);
  int waveformIndex = atoi(argv[13]);
  int seed;
  if (argc==15)
    {
      seed = atoi(argv[14]);
    }
  else
    {
      seed = -1;
    }

  double bandwidth = 20; // MHz
//  double bandwidth = 1.4; // MHz
  int etilt; //degrees
  double penetrationLossMeanIndoor = 20; // dB
  double penetrationLossMeanOutdoor = 0; // dB
  double penetrationLossStdDev = 0; //dB
  double carrierFreq; //MHz
  double txPower = 46; // dBm
//  double txPower = 33.8; // dBm
  double antennaHeight = 25; // m
  double avgBuildingHeight = 20; // m
  double antennaGain = 8; // dBi
  double antennaAttenuation = 20; // dB
  double horizontalBeamwidth3db = 65; //degrees
  double UENoiseFigure = 9; // dB
  double handoverMargin = 3; // dB
  double cqiReportingInterval = 1; // TTI
  double shadowingStddev = 10; //dB
  int channelModel = 1;
  int nbCell = 20; // number of sites in the simulation
  int nbActiveCell = 1; // number of sites with active users
  int nbSector = 3; // number of sectors per site
  double BSNoiseFigure = 7; // dB
  bool handover = true;
  double indoorUeFraction = 0;
  double minDist = 0.025; // in km
  int enbTx = nbTx;
  int ueRx = nbRx;
  double vBeamWidth = 65;
  double BSFeederLoss = 2;


//  double isd; // km
  if(environment=="suburban")
    {
      //isd = 0.25;
      etilt = 14;
      channelModel = 4;
      carrierFreq = 2000;
    }
  else if(environment=="rural")
    {
      //isd = 0.5;
      etilt = 10;
      channelModel = 4;
      carrierFreq = 800;
    }
  else
    {
      cout << "ERROR: invalid environment selected" << endl;
    }

  LtePhy::WaveformType waveform;
  switch (waveformIndex)
    {
    case 0:
      waveform = LtePhy::WAVEFORM_TYPE_OFDM;
      break;

    case 1:
      waveform = LtePhy::WAVEFORM_TYPE_POFDM;
      break;

    case 2:
      waveform = LtePhy::WAVEFORM_TYPE_IDEAL_NO_DOPPLER;
      break;

    default:
      cout << "Error: invalid waveform selected" << endl;
      exit(1);
      break;
    }


  // define simulation times
  double flow_duration = duration;

  int nbApplication = 1; // number of application flows per user

  // define channel model
  ChannelRealization::ChannelModel model;

  switch(channelModel)
    {
    case 0:
      model = ChannelRealization::CHANNEL_MODEL_MACROCELL_URBAN_IMT;
      break;
    case 1:
      model = ChannelRealization::CHANNEL_MODEL_MACROCELL_RURAL_IMT;
      break;
    case 2:
      model = ChannelRealization::CHANNEL_MODEL_3GPP_CASE1;
      break;
    case 3:
      model = ChannelRealization::CHANNEL_MODEL_MACROCELL_RURAL;
      break;
    case 4:
      model = ChannelRealization::CHANNEL_MODEL_MACROCELL_URBAN_IMT_3D;
      break;
    default:
      model = ChannelRealization::CHANNEL_MODEL_MACROCELL_URBAN;
      break;
    }

  // CREATE COMPONENT MANAGER
  Simulator *simulator = Simulator::Init();
  FrameManager *frameManager = FrameManager::Init();
  NetworkManager* networkManager = NetworkManager::Init();

  // CONFIGURE SEED
  if (seed >= 0)
    {
      srand (seed);
    }
  else
    {
      srand (time(nullptr));
    }
  cout << "Simulation with SEED = " << seed << endl;

  double radius = isd / sqrt(3); // define cell radius from inter site distance

  // SET SCHEDULING ALLOCATION SCHEME
  ENodeB::DLSchedulerType downlink_scheduler_type;
  switch (sched_type)
    {
    case 1:
      downlink_scheduler_type = ENodeB::DLScheduler_TYPE_PROPORTIONAL_FAIR;
      cout << "Scheduler PF "<< endl;
      break;
    case 2:
      downlink_scheduler_type = ENodeB::DLScheduler_TYPE_MLWDF;
      cout << "Scheduler MLWDF "<< endl;
      break;
    case 3:
      downlink_scheduler_type = ENodeB::DLScheduler_TYPE_EXP;
      cout << "Scheduler EXP "<< endl;
      break;
    case 4:
      downlink_scheduler_type = ENodeB::DLScheduler_TYPE_FLS;
      cout << "Scheduler FLS "<< endl;
      break;
    case 5:
      downlink_scheduler_type = ENodeB::DLScheduler_EXP_RULE;
      cout << "Scheduler EXP_RULE "<< endl;
      break;
    case 6:
      downlink_scheduler_type = ENodeB::DLScheduler_LOG_RULE;
      cout << "Scheduler LOG RULE "<< endl;
      break;
    case 7:
      downlink_scheduler_type = ENodeB::DLScheduler_TYPE_MAXIMUM_THROUGHPUT;
      cout << "Scheduler MT "<< endl;
      break;
    case 8:
      downlink_scheduler_type = ENodeB::DLScheduler_TYPE_ROUND_ROBIN;
      cout << "Scheduler RR "<< endl;
      break;
    default:
      downlink_scheduler_type = ENodeB::DLScheduler_TYPE_PROPORTIONAL_FAIR;
      break;
    }

  // SET FRAME STRUCTURE
  frameManager->SetFrameStructure(FrameManager::FRAME_STRUCTURE_FDD);


  // SET WRAPAROUND
  networkManager->SetWrapAroundType(NetworkManager::WRAPAROUND_X_AXIS);
  networkManager->SetWrapAroundDistanceX(nbCell/4*isd*1000);

  // CREATE CELLS
  vector <Cell*> *cells = new vector <Cell*>;
  for (int i = 0; i < nbCell; i++)
    {
      CartesianCoordinates center;
      center.SetCoordinateX((i/4)*isd*1000 + (i%2)*(isd/2)*1000);
      center.SetCoordinateY(-1.5*isd*1000 + (i%4)*isd*1000);

      Cell *c = new Cell (i, radius, minDist, center.GetCoordinateX (), center.GetCoordinateY ());
      cells->push_back (c);
      networkManager->GetCellContainer ()->push_back (c);

      cout << "Created Cell, id " << c->GetIdCell ()
                <<", position: " << c->GetCellCenterPosition ()->GetCoordinateX ()
                << " " << c->GetCellCenterPosition ()->GetCoordinateY ()
                << " radius " << radius << endl;
    }

  for (int i = 0; i < nbCell; i++)
    {
      for (int j=0; j<nbSector; j++)
        {
          // CREATE CHANNELS and propagation loss model
          LteChannel *dlCh = new LteChannel ();
          LteChannel *ulCh = new LteChannel ();

          // CREATE SPECTRUM
          BandwidthManager* spectrum = new BandwidthManager (bandwidth, bandwidth, 0, 0);

          //Create ENodeB
          ENodeB* enb = new ENodeB (i*3+j, cells->at (i));
          enb->GetPhy ()->SetDlChannel (dlCh);
          enb->GetPhy ()->SetUlChannel (ulCh);
          enb->GetPhy ()->SetNoiseFigure(BSNoiseFigure);
          enb->GetPhy ()->SetCarrierFrequency(carrierFreq);
          enb->GetPhy ()->SetTxAntennas(enbTx);
          enb->GetPhy ()->SetTxPower(txPower);
          enb->GetPhy ()->SetBandwidthManager (spectrum);
          enb->GetPhy ()->SetHeight(antennaHeight);
          enb->GetPhy ()->SetAverageBuildingHeight(avgBuildingHeight);
          enb->GetPhy ()->SetWaveformType(waveform);
          enb->GetProtocolStack ()->GetRrcEntity ()->GetHandoverEntity ()->GetHoManager ()->SetHandoverMargin(handoverMargin);
          // Set Antenna Parameters
          if(nbSector==1)
            {
              enb->GetPhy ()->GetAntennaParameters ()->SetType(LtePhy::AntennaParameters::ANTENNA_TYPE_OMNIDIRECTIONAL);
            }
          else if(nbSector==3)
            {
              enb->GetPhy ()->GetAntennaParameters ()->SetType(LtePhy::AntennaParameters::ANTENNA_TYPE_TRI_SECTOR);
              enb->GetPhy ()->GetAntennaParameters ()->SetEtilt(etilt);
              enb->GetPhy ()->GetAntennaParameters ()->SetHorizontalBeamwidth3db(horizontalBeamwidth3db);
              enb->GetPhy ()->GetAntennaParameters ()->SetVerticalBeamwidth3db(vBeamWidth);
              enb->GetPhy ()->GetAntennaParameters ()->SetGain(antennaGain);
              enb->GetPhy ()->GetAntennaParameters ()->SetMaxHorizontalAttenuation(antennaAttenuation);
              enb->GetPhy ()->GetAntennaParameters ()->SetMaxVerticalAttenuation(antennaAttenuation);
              enb->GetPhy ()->GetAntennaParameters ()->SetBearing(120*j);
            }
          else
            {
              cout << "Error: unsupported number of cell sectors (" << nbSector << ")" <<endl;
              exit(1);
            }
          enb->GetPhy ()->GetAntennaParameters ()->SetFeederLoss(BSFeederLoss);
          enb->GetMacEntity ()->SetDefaultDlTxMode(txMode);
          ulCh->AddDevice (enb);
//          if (enb->GetCell()->GetIdCell() < nbActiveCell)
//          if (enb->GetIDNetworkNode() < nbActiveCell)
          {
            enb->SetDLScheduler (downlink_scheduler_type);
            enb->GetDLScheduler()->SetMaxUsersPerRB(nbMu);
          }
          networkManager->GetENodeBContainer ()->push_back (enb);

        }
    }

  //Create GW
  Gateway *gw = new Gateway ();
  networkManager->GetGatewayContainer ()->push_back (gw);


  cout << "Creating " << nbUE << " user terminals" << endl;

  //Define Application Container
//  FTP2 ftpApplication[nbApplication*nbUE];
  InfiniteBuffer ftpApplication[nbApplication*nbUE];
  int ftpApplicationID = 0;
  int destinationPort = 101;
  int applicationID = 0;
  CartesianCoordinates center = CartesianCoordinates(0,0);

  int idUE = nbCell*nbSector;

  //Create UEs
  for (int n = 0; n < nbUE; n++)
    {
      //ue's random position
      double posX, posY;
      CartesianCoordinates position;
      posX=n*10;
      posY=0;
      position = CartesianCoordinates(posX,posY);
//          for(int i=0; i<nbCell; i++)
//            {
//              double distance = cells->at(i)->GetCellCenterPosition ()->GetDistance(posX/1000,posY/1000);
//              if (distance<minDist)
//                {
//                  isNearToCellCenter = true;
//                }
//            }
//cout << "X " << posX << " Y " << posY << " Azimuth " << position.GetPolarAzimut(&center) << endl;

//      double speedDirection = (double)(rand() %360) * ((2*M_PI)/360);
      double speedDirection = 0;

      ENodeB* enb = networkManager->GetENodeBContainer ()->at(0);

      UserEquipment* ue = new UserEquipment (idUE,
                                             posX, posY, speed, speedDirection,
                                             cells->at (0),
                                             enb,
                                             handover,
                                             //Mobility::RANDOM_WALK
                                             //Mobility::RANDOM_DIRECTION
                                             Mobility::LINEAR_MOVEMENT
//                                      Mobility::CONSTANT_POSITION
                                            );

      cout << "Created UE - id " << idUE << " position " << posX << " " << posY << endl;

//      Simulator::Init()->Schedule(0.003, &Mobility::SetHandover, ue->GetMobilityModel(), false);
      ue->GetProtocolStack()->GetRrcEntity()->GetHandoverEntity()->SetDetachTime(0.002);

      if (use_srta_pi==1)
        {
          ue->GetPhy()->SetSrtaPi(true);
        }
      LteChannel* dlCh = enb->GetPhy ()->GetDlChannel ();
      LteChannel* ulCh = enb->GetPhy ()->GetUlChannel ();

      ue->GetPhy ()->SetDlChannel (dlCh);
      ue->GetPhy ()->SetUlChannel (ulCh);
      ue->GetPhy ()->SetNoiseFigure(UENoiseFigure);
      ue->GetPhy ()->SetRxAntennas(ueRx);
      if((double)rand()/RAND_MAX < indoorUeFraction)
        {
          ue->SetIndoorFlag(true);
        }

      WidebandCqiManager *cqiManager = new WidebandCqiManager ();
//      FullbandCqiManager *cqiManager = new FullbandCqiManager ();
      cqiManager->SetCqiReportingMode (CqiManager::PERIODIC);
      cqiManager->SetReportingInterval (cqiReportingInterval);
      cqiManager->SetDevice (ue);
      ue->SetCqiManager (cqiManager);

      WidebandCqiEesmErrorModel *errorModel = new WidebandCqiEesmErrorModel ();
      ue->GetPhy ()->SetErrorModel (errorModel);

      networkManager->GetUserEquipmentContainer ()->push_back (ue);

      // register ue to the enb
      enb->RegisterUserEquipment (ue);
      // define the channel realizations
      ENodeB* prevEnb = nullptr;
      ChannelRealization *prev_c_dl, *prev_c_ul;
      for ( auto enb : *networkManager->GetENodeBContainer () )
        {
          ChannelRealization* c_dl = new ChannelRealization (enb, ue, model);
          enb->GetPhy ()->GetDlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_dl);
//          ChannelRealization* c_ul = new ChannelRealization (ue, enb, model);
//          enb->GetPhy ()->GetUlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_ul);

          c_dl->SetShadowingStddev(shadowingStddev);
//          c_ul->SetShadowingStddev(shadowingStddev);

          if(ue->IsIndoor())
            {
              c_dl->SetPenetrationLossMean(penetrationLossMeanIndoor);
//              c_ul->SetPenetrationLossMean(penetrationLossMeanIndoor);
            }
          else
            {
              c_dl->SetPenetrationLossMean(penetrationLossMeanOutdoor);
//              c_ul->SetPenetrationLossMean(penetrationLossMeanOutdoor);
            }
          c_dl->SetPenetrationLossStdDev(penetrationLossStdDev);
//          c_ul->SetPenetrationLossStdDev(penetrationLossStdDev);

          // syncronize LoS state and shadowing for eNBs at the same site
          if (prevEnb != nullptr && enb->GetCell()->GetIdCell() == prevEnb->GetCell()->GetIdCell())
            {
              c_dl->SetLoSState(prev_c_dl->GetLoSState());
//              c_ul->SetLoSState(prev_c_ul->GetLoSState());
              c_dl->SetShadowing(prev_c_dl->GetShadowing());
//              c_ul->SetShadowing(prev_c_ul->GetShadowing());
            }
          prevEnb = enb;
          prev_c_dl = c_dl;
//          prev_c_ul = c_ul;
        }


      // CREATE DOWNLINK APPLICATION FOR THIS UE
      double start_time = /*0.001*idUE*/ + 0.01;
      double duration_time = start_time + flow_duration;

      // *** ftp application
      for (int j = 0; j < nbApplication; j++)
        {
          // create application
          ftpApplication[ftpApplicationID].SetSource (gw);
          ftpApplication[ftpApplicationID].SetDestination (ue);
          ftpApplication[ftpApplicationID].SetApplicationID (applicationID);
          ftpApplication[ftpApplicationID].SetStartTime(start_time);
          ftpApplication[ftpApplicationID].SetStopTime(duration_time);


          // create qos parameters
          QoSParameters *qosParameters = new QoSParameters ();
          ftpApplication[ftpApplicationID].SetQoSParameters (qosParameters);


          //create classifier parameters
          ClassifierParameters *cp = new ClassifierParameters (gw->GetIDNetworkNode(),
              ue->GetIDNetworkNode(),
              0,
              destinationPort,
              TransportProtocol::TRANSPORT_PROTOCOL_TYPE_UDP);
          ftpApplication[ftpApplicationID].SetClassifierParameters (cp);

          cout << "CREATED BE APPLICATION, ID " << applicationID << endl;

          //update counter
          destinationPort++;
          applicationID++;
          ftpApplicationID++;
        }
      idUE++;
    }

  simulator->SetStop(duration);
  simulator->Run ();
}

