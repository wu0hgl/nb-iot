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
 * Author: Francesco Capozzi <f.capozzi@poliba.it>
 */

#include "../channel/LteChannel.h"
#include "../phy/enb-lte-phy.h"
#include "../phy/ue-lte-phy.h"
#include "../core/spectrum/bandwidth-manager.h"
#include "../networkTopology/Cell.h"
#include "../protocolStack/packet/packet-burst.h"
#include "../protocolStack/packet/Packet.h"
#include "../core/eventScheduler/simulator.h"
#include "../flows/application/InfiniteBuffer.h"
#include "../flows/application/VoIP.h"
#include "../flows/application/CBR.h"
#include "../flows/application/TraceBased.h"
#include "../device/IPClassifier/ClassifierParameters.h"
#include "../flows/QoS/QoSParameters.h"
#include "../flows/QoS/QoSForEXP.h"
#include "../flows/QoS/QoSForFLS.h"
#include "../flows/QoS/QoSForM_LWDF.h"
#include "../componentManagers/FrameManager.h"
#include "../utility/RandomVariable.h"
#include "../utility/UsersDistribution.h"
#include "../utility/IndoorScenarios.h"
#include "../load-parameters.h"
#include "../device/HeNodeB.h"
#include <iostream>
#include <queue>
#include <fstream>
#include <stdlib.h>
#include <cstring>


static void SingleCellWithStreets (int argc, char *argv[])
{
  int nbStreets = atoi(argv[2]);
  double radius = atof(argv[3]);
  int nbUE = atoi(argv[4]);
  int nbFemtoUE = atoi(argv[5]);
  int nbVoIP = atoi(argv[6]);
  int nbVideo = atoi(argv[7]);
  int nbBE = atoi(argv[8]);
  int nbCBR = atoi(argv[9]);
  int sched_type = atoi(argv[10]);
  int frame_struct = atoi(argv[11]);
  int speed = atoi(argv[12]);
  double maxDelay = atof(argv[13]);
  int videoBitRate = atoi(argv[14]);
  int seed;
  if (argc==16)
    {
      seed = atoi(argv[15]);
    }
  else
    {
      seed = -1;
    }

  int nbCell = 1;

  // define simulation times
  double duration = 100;
  double flow_duration = 100;

  int cluster = 3;
  double bandwidth = 5;

  // CREATE COMPONENT MANAGER
  Simulator *simulator = Simulator::Init();
  FrameManager *frameManager = FrameManager::Init();
  NetworkManager* nm = NetworkManager::Init();

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
  FrameManager::FrameStructure frame_structure;
  switch (frame_struct)
    {
    case 1:
      frame_structure = FrameManager::FRAME_STRUCTURE_FDD;
      break;
    case 2:
      frame_structure = FrameManager::FRAME_STRUCTURE_TDD;
      break;
    default:
      cout << "Error: invalid frame structure specified!" << endl;
      exit(1);
    }
  frameManager->SetFrameStructure(frame_structure);




  //create macro-cells
  vector <Cell*> *cells = new vector <Cell*>;
  for (int i = 0; i < nbCell; i++)
    {
      CartesianCoordinates center =
        GetCartesianCoordinatesForCell(i, radius * 1000.);


      Cell *c = new Cell (i, radius, 0.035, center.GetCoordinateX (), center.GetCoordinateY ());
      cells->push_back (c);
      nm->GetCellContainer ()->push_back (c);

      cout << "Created Cell, id " << c->GetIdCell ()
                <<", position: " << c->GetCellCenterPosition ()->GetCoordinateX ()
                << " " << c->GetCellCenterPosition ()->GetCoordinateY () << endl;
    }

  //////////////////////
  //create femto-cells
  //
  int streetID = 0;
  int street_width = 30; //[m]
  int interBuilding_distance = 15; //[m]
  int nbOfBuildingInStreet = 10;
  double apartment_side = 10;

  CartesianCoordinates* streetCenter = new CartesianCoordinates(100, 100);
  double theta = 0.0;

  int femtoCellsInBuilding = 25;
  int nbFemtoCells = nbStreets * 2 * nbOfBuildingInStreet * femtoCellsInBuilding;
  int firstFemtoinBuildingID = nbCell;
  int buildingtype = 0; // 0: 5x5 grid

  nm->CreateStreet( streetID,
                    streetCenter, theta, buildingtype,
                    nbOfBuildingInStreet, street_width, interBuilding_distance,
                    femtoCellsInBuilding, apartment_side, firstFemtoinBuildingID );


  //
  //
  ///////////////////////////

  vector <BandwidthManager*> spectrums = RunFrequencyReuseTechniques (nbCell, cluster, bandwidth);

  BandwidthManager* femto_spectrums = spectrums.at(0);
// vector <BandwidthManager*> femto_spectrums = RunFrequencyReuseTechniques (nbCell, cluster, bandwidth);

  //Create a set of a couple of channels
  vector <LteChannel*> *dlChannels = new vector <LteChannel*>;
  vector <LteChannel*> *ulChannels = new vector <LteChannel*>;
  for (int i= 0; i < nbCell + nbFemtoCells; i++)
    {
      LteChannel *dlCh = new LteChannel ();
      dlCh->SetChannelId (i);
      dlChannels->push_back (dlCh);

      LteChannel *ulCh = new LteChannel ();
      ulCh->SetChannelId (i);
      ulChannels->push_back (ulCh);
    }


  //create eNBs
  vector <ENodeB*> *eNBs = new vector <ENodeB*>;
  for (int i = 0; i < nbCell; i++)
    {
      ENodeB* enb = new ENodeB (i, cells->at (i));
      enb->GetPhy ()->SetDlChannel (dlChannels->at (i));
      enb->GetPhy ()->SetUlChannel (ulChannels->at (i));

      enb->SetDLScheduler (downlink_scheduler_type);

      enb->GetPhy ()->SetBandwidthManager (spectrums.at (i));

      cout << "Created enb, id " << enb->GetIDNetworkNode()
                << ", cell id " << enb->GetCell ()->GetIdCell ()
                <<", position: " << enb->GetMobilityModel ()->GetAbsolutePosition ()->GetCoordinateX ()
                << " " << enb->GetMobilityModel ()->GetAbsolutePosition ()->GetCoordinateY ()
                << ", channels id " << enb->GetPhy ()->GetDlChannel ()->GetChannelId ()
                << enb->GetPhy ()->GetUlChannel ()->GetChannelId ()  << endl;

      spectrums.at (i)->Print ();


      ulChannels->at (i)->AddDevice(enb);


      nm->GetENodeBContainer ()->push_back (enb);
      eNBs->push_back (enb);
    }

  //create Home eNBs
  vector <Femtocell*> *femtocells = nm->GetFemtoCellContainer();
  for (int i = nbCell; i < nbCell + nbFemtoCells; i++)
    {
      HeNodeB* enb = new HeNodeB (i, femtocells->at (i-nbCell));
      enb->GetPhy ()->SetDlChannel (dlChannels->at (i));
      enb->GetPhy ()->SetUlChannel (ulChannels->at (i));

      enb->SetDLScheduler (downlink_scheduler_type);

      enb->GetPhy ()->SetBandwidthManager (femto_spectrums);

      cout << "Created Home enb, id " << enb->GetIDNetworkNode()
                << ", cell id " << enb->GetCell ()->GetIdCell ()
                <<", position: " << enb->GetMobilityModel ()->GetAbsolutePosition ()->GetCoordinateX ()
                << " " << enb->GetMobilityModel ()->GetAbsolutePosition ()->GetCoordinateY ()
                << ", channels id " << enb->GetPhy ()->GetDlChannel ()->GetChannelId ()
                << enb->GetPhy ()->GetUlChannel ()->GetChannelId ()  << endl;

      femto_spectrums->Print ();


      ulChannels->at (i)->AddDevice(enb);

      nm->GetHomeENodeBContainer()->push_back (enb);
    }


  int totalNbUE = nbCell*nbUE + nbFemtoCells*nbFemtoUE;
  int totalNbCell = nbCell + nbFemtoCells;

  //Define Application Container
  VoIP VoIPApplication[ nbVoIP*totalNbUE ];
  TraceBased VideoApplication[ nbVideo*totalNbUE ];
  InfiniteBuffer BEApplication[ nbBE*totalNbUE ];
  CBR CBRApplication[ nbCBR*totalNbUE ];
  int voipApplication = 0;
  int videoApplication = 0;
  int cbrApplication = 0;
  int beApplication = 0;
  int destinationPort = 101;
  int applicationID = 0;



  //Create GW
  Gateway *gw = new Gateway ();
  nm->GetGatewayContainer ()->push_back (gw);


  // Users in MACRO CELL
  //nbUE is the number of users that are into each cell at the beginning of the simulation
  int idUE = totalNbCell;
  for (int j = 0; j < nbCell; j++)
    {

      //users are distributed uniformly into a cell
      vector<CartesianCoordinates*> *positions = GetUniformUsersDistribution (j, nbUE);

      //Create UEs
      for (int i = 0; i < nbUE; i++)
        {
          //ue's random position
          double posX = positions->at (idUE - totalNbCell)->GetCoordinateX ();
          double posY = positions->at (idUE - totalNbCell)->GetCoordinateY ();
          double speedDirection = (double)(rand() %360) * ((2*M_PI)/360);;

          UserEquipment* ue = new UserEquipment (idUE,
                                                 posX, posY, speed, speedDirection,
                                                 cells->at (j),
                                                 eNBs->at (j),
                                                 1, //HO activated!
                                                 Mobility::RANDOM_DIRECTION);

          cout << "Created UE - id " << idUE << " position " << posX << " " << posY
                    << ", cell " <<  ue->GetCell ()->GetIdCell ()
                    << ", target enb " << ue->GetTargetNode ()->GetIDNetworkNode () << endl;

          ue->GetPhy ()->SetDlChannel (eNBs->at (j)->GetPhy ()->GetDlChannel ());
          ue->GetPhy ()->SetUlChannel (eNBs->at (j)->GetPhy ()->GetUlChannel ());

          ue->SetIndoorFlag(false);

          FullbandCqiManager *cqiManager = new FullbandCqiManager ();
          cqiManager->SetCqiReportingMode (CqiManager::PERIODIC);
          cqiManager->SetReportingInterval (1);
          cqiManager->SetDevice (ue);
          ue->SetCqiManager (cqiManager);

          nm->GetUserEquipmentContainer ()->push_back (ue);

          // register ue to the enb
          eNBs->at (j)->RegisterUserEquipment (ue);
          // define the channel realization
          ChannelRealization* c_dl = new ChannelRealization (eNBs->at (j), ue, ChannelRealization::CHANNEL_MODEL_MACROCELL_URBAN);
          eNBs->at (j)->GetPhy ()->GetDlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_dl);
          ChannelRealization* c_ul = new ChannelRealization (ue, eNBs->at (j), ChannelRealization::CHANNEL_MODEL_MACROCELL_URBAN);
          eNBs->at (j)->GetPhy ()->GetUlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_ul);
          for (int k = 0; k < nbFemtoCells; k++)
            {
              c_dl = new ChannelRealization (nm->GetHomeENodeBContainer()->at (k), ue, ChannelRealization::CHANNEL_MODEL_FEMTOCELL_URBAN);
              nm->GetHomeENodeBContainer()->at (k)->GetPhy ()->GetDlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_dl);
              c_ul = new ChannelRealization (ue, nm->GetHomeENodeBContainer()->at (k), ChannelRealization::CHANNEL_MODEL_FEMTOCELL_URBAN);
              nm->GetHomeENodeBContainer()->at (k)->GetPhy ()->GetUlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_ul);
            }

          idUE++;
        }
    }

  // Users in FEMTO CELLS
  //nbUE is the number of users that are into each cell at the beginning of the simulation
  //idUE = nbCell*nbUE;
  vector <HeNodeB*> *HeNBs = nm->GetHomeENodeBContainer();
  for (int j = 0; j < nbFemtoCells; j++)
    {
      int idCell = j + nbCell;
      //users are distributed uniformly into a femto cell
      vector<CartesianCoordinates*> *positions = GetUniformUsersDistributionInFemtoCell (idCell, nbFemtoUE);

      //Create UEs
      for (int i = 0; i < nbFemtoUE; i++)
        {
          //ue's random position
          double posX = positions->at (i)->GetCoordinateX ();
          double posY = positions->at (i)->GetCoordinateY ();
          double speedDirection = (double)(rand() %360) * ((2*M_PI)/360);;

          UserEquipment* ue = new UserEquipment (idUE,
                                                 posX, posY, speed, speedDirection,
                                                 femtocells->at (j),
                                                 HeNBs->at (j),
                                                 1, //HO activated!
                                                 Mobility::RANDOM_DIRECTION);

          cout << "Created UE in femto-cell - id " << idUE << " position " << posX << " " << posY
                    << ", cell " <<  ue->GetCell ()->GetIdCell ()
                    << ", target enb " << ue->GetTargetNode ()->GetIDNetworkNode () << endl;

          ue->GetPhy ()->SetDlChannel (HeNBs->at (j)->GetPhy ()->GetDlChannel ());
          ue->GetPhy ()->SetUlChannel (HeNBs->at (j)->GetPhy ()->GetUlChannel ());

          ue->SetIndoorFlag(true);

          FullbandCqiManager *cqiManager = new FullbandCqiManager ();
          cqiManager->SetCqiReportingMode (CqiManager::PERIODIC);
          cqiManager->SetReportingInterval (1);
          cqiManager->SetDevice (ue);
          ue->SetCqiManager (cqiManager);

          nm->GetUserEquipmentContainer ()->push_back (ue);

          // register ue to the enb
          HeNBs->at (j)->RegisterUserEquipment (ue);
          // define the channel realizations
          ChannelRealization* c_dl = new ChannelRealization (eNBs->at (j), ue, ChannelRealization::CHANNEL_MODEL_MACROCELL_URBAN);
          eNBs->at (j)->GetPhy ()->GetDlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_dl);
          ChannelRealization* c_ul = new ChannelRealization (ue, eNBs->at (j), ChannelRealization::CHANNEL_MODEL_MACROCELL_URBAN);
          eNBs->at (j)->GetPhy ()->GetUlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_ul);
          for (int k = 0; k < nbFemtoCells; k++)
            {
              c_dl = new ChannelRealization (nm->GetHomeENodeBContainer()->at (k), ue, ChannelRealization::CHANNEL_MODEL_FEMTOCELL_URBAN);
              nm->GetHomeENodeBContainer()->at (k)->GetPhy ()->GetDlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_dl);
              c_ul = new ChannelRealization (ue, nm->GetHomeENodeBContainer()->at (k), ChannelRealization::CHANNEL_MODEL_FEMTOCELL_URBAN);
              nm->GetHomeENodeBContainer()->at (k)->GetPhy ()->GetUlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_ul);
            }

          idUE++;
        }
    }

  for (auto ue : *nm->GetUserEquipmentContainer ())
    {
      // CREATE DOWNLINK APPLICATION FOR THIS UE
      double start_time = 0.1; // + GetRandomVariable (5.);
      double duration_time = start_time + flow_duration;
      // *** voip application
      for (int j = 0; j < nbVoIP; j++)
        {
          // create application
          VoIPApplication[voipApplication].SetSource (gw);
          VoIPApplication[voipApplication].SetDestination (ue);
          VoIPApplication[voipApplication].SetApplicationID (applicationID);
          VoIPApplication[voipApplication].SetStartTime(start_time);
          VoIPApplication[voipApplication].SetStopTime(duration_time);

          // create qos parameters
          if (downlink_scheduler_type == ENodeB::DLScheduler_TYPE_FLS)
            {
              QoSForFLS *qos = new QoSForFLS ();
              qos->SetMaxDelay (maxDelay);
              if (maxDelay == 0.1)
                {
                  cout << "Target Delay = 0.1 s, M = 9" << endl;
                  qos->SetNbOfCoefficients (9);
                }
              else if (maxDelay == 0.08)
                {
                  cout << "Target Delay = 0.08 s, M = 7" << endl;
                  qos->SetNbOfCoefficients (7);
                }
              else if (maxDelay == 0.06)
                {
                  cout << "Target Delay = 0.06 s, M = 5" << endl;
                  qos->SetNbOfCoefficients (5);
                }
              else if (maxDelay == 0.04)
                {
                  cout << "Target Delay = 0.04 s, M = 3" << endl;
                  qos->SetNbOfCoefficients (3);
                }
              else
                {
                  cout << "ERROR: target delay is not available"<< endl;
                  return;
                }

              VoIPApplication[voipApplication].SetQoSParameters (qos);
            }
          else if (downlink_scheduler_type == ENodeB::DLScheduler_TYPE_EXP)
            {
              QoSForEXP *qos = new QoSForEXP ();
              qos->SetMaxDelay (maxDelay);
              VoIPApplication[voipApplication].SetQoSParameters (qos);
            }
          else if (downlink_scheduler_type == ENodeB::DLScheduler_TYPE_MLWDF)
            {
              QoSForM_LWDF *qos = new QoSForM_LWDF ();
              qos->SetMaxDelay (maxDelay);
              VoIPApplication[voipApplication].SetQoSParameters (qos);
            }
          else
            {
              QoSParameters *qos = new QoSParameters ();
              qos->SetMaxDelay (maxDelay);
              VoIPApplication[voipApplication].SetQoSParameters (qos);
            }


          //create classifier parameters
          ClassifierParameters *cp = new ClassifierParameters (gw->GetIDNetworkNode(),
              ue->GetIDNetworkNode(),
              0,
              destinationPort,
              TransportProtocol::TRANSPORT_PROTOCOL_TYPE_UDP);
          VoIPApplication[voipApplication].SetClassifierParameters (cp);

          cout << "CREATED VOIP APPLICATION, ID " << applicationID << endl;

          //update counter
          destinationPort++;
          applicationID++;
          voipApplication++;
        }


      // *** video application
      for (int j = 0; j < nbVideo; j++)
        {
          // create application
          VideoApplication[videoApplication].SetSource (gw);
          VideoApplication[videoApplication].SetDestination (ue);
          VideoApplication[videoApplication].SetApplicationID (applicationID);
          VideoApplication[videoApplication].SetStartTime(start_time);
          VideoApplication[videoApplication].SetStopTime(duration_time);

          switch (videoBitRate)
            {
            case 128:
              {
                VideoApplication[videoApplication].LoadInternalTrace(&foreman_h264_128k);
//                  VideoApplication[videoApplication].LoadInternalTrace(&highway_h264_128k);
//                  VideoApplication[videoApplication].LoadInternalTrace(&mobile_h264_128k);
                cout << "  selected video @ 128k"<< endl;
                break;
              }
            case 242:
              {
                VideoApplication[videoApplication].LoadInternalTrace(&foreman_h264_242k);
                cout << "  selected video @ 242k"<< endl;
                break;
              }
            case 440:
              {
                VideoApplication[videoApplication].LoadInternalTrace(&foreman_h264_440k);
                cout << "  selected video @ 440k"<< endl;
                break;
              }
            default:
              {
                cout << "  Unsupported video bitrate!"<< endl;
                exit(1);
              }
            }

          // create qos parameters
          if (downlink_scheduler_type == ENodeB::DLScheduler_TYPE_FLS)
            {
              QoSForFLS *qos = new QoSForFLS ();
              qos->SetMaxDelay (maxDelay);
              if (maxDelay == 0.1)
                {
                  cout << "Target Delay = 0.1 s, M = 9" << endl;
                  qos->SetNbOfCoefficients (9);
                }
              else if (maxDelay == 0.08)
                {
                  cout << "Target Delay = 0.08 s, M = 7" << endl;
                  qos->SetNbOfCoefficients (7);
                }
              else if (maxDelay == 0.06)
                {
                  cout << "Target Delay = 0.06 s, M = 5" << endl;
                  qos->SetNbOfCoefficients (5);
                }
              else if (maxDelay == 0.04)
                {
                  cout << "Target Delay = 0.04 s, M = 3" << endl;
                  qos->SetNbOfCoefficients (3);
                }
              else
                {
                  cout << "ERROR: target delay is not available"<< endl;
                  return;
                }

              VideoApplication[videoApplication].SetQoSParameters (qos);
            }
          else if (downlink_scheduler_type == ENodeB::DLScheduler_TYPE_EXP)
            {
              QoSForEXP *qos = new QoSForEXP ();
              qos->SetMaxDelay (maxDelay);
              VideoApplication[videoApplication].SetQoSParameters (qos);
            }
          else if (downlink_scheduler_type == ENodeB::DLScheduler_TYPE_MLWDF)
            {
              QoSForM_LWDF *qos = new QoSForM_LWDF ();
              qos->SetMaxDelay (maxDelay);
              VideoApplication[videoApplication].SetQoSParameters (qos);
            }
          else
            {
              QoSParameters *qos = new QoSParameters ();
              qos->SetMaxDelay (maxDelay);
              VideoApplication[videoApplication].SetQoSParameters (qos);
            }


          //create classifier parameters
          ClassifierParameters *cp = new ClassifierParameters (gw->GetIDNetworkNode(),
              ue->GetIDNetworkNode(),
              0,
              destinationPort,
              TransportProtocol::TRANSPORT_PROTOCOL_TYPE_UDP);
          VideoApplication[videoApplication].SetClassifierParameters (cp);

          cout << "CREATED VIDEO APPLICATION, ID " << applicationID << endl;

          //update counter
          destinationPort++;
          applicationID++;
          videoApplication++;
        }

      // *** be application
      for (int j = 0; j < nbBE; j++)
        {
          // create application
          BEApplication[beApplication].SetSource (gw);
          BEApplication[beApplication].SetDestination (ue);
          BEApplication[beApplication].SetApplicationID (applicationID);
          BEApplication[beApplication].SetStartTime(start_time);
          BEApplication[beApplication].SetStopTime(duration_time);


          // create qos parameters
          QoSParameters *qosParameters = new QoSParameters ();
          BEApplication[beApplication].SetQoSParameters (qosParameters);


          //create classifier parameters
          ClassifierParameters *cp = new ClassifierParameters (gw->GetIDNetworkNode(),
              ue->GetIDNetworkNode(),
              0,
              destinationPort,
              TransportProtocol::TRANSPORT_PROTOCOL_TYPE_UDP);
          BEApplication[beApplication].SetClassifierParameters (cp);

          cout << "CREATED BE APPLICATION, ID " << applicationID << endl;

          //update counter
          destinationPort++;
          applicationID++;
          beApplication++;
        }

      // *** cbr application
      for (int j = 0; j < nbCBR; j++)
        {
          // create application
          CBRApplication[cbrApplication].SetSource (gw);
          CBRApplication[cbrApplication].SetDestination (ue);
          CBRApplication[cbrApplication].SetApplicationID (applicationID);
          CBRApplication[cbrApplication].SetStartTime(start_time);
          CBRApplication[cbrApplication].SetStopTime(duration_time);
          CBRApplication[cbrApplication].SetInterval (0.04);
          CBRApplication[cbrApplication].SetSize (5);

          // create qos parameters
          QoSParameters *qosParameters = new QoSParameters ();
          qosParameters->SetMaxDelay (maxDelay);

          CBRApplication[cbrApplication].SetQoSParameters (qosParameters);


          //create classifier parameters
          ClassifierParameters *cp = new ClassifierParameters (gw->GetIDNetworkNode(),
              ue->GetIDNetworkNode(),
              0,
              destinationPort,
              TransportProtocol::TRANSPORT_PROTOCOL_TYPE_UDP);
          CBRApplication[cbrApplication].SetClassifierParameters (cp);

          cout << "CREATED CBR APPLICATION, ID " << applicationID << endl;

          //update counter
          destinationPort++;
          applicationID++;
          cbrApplication++;
        }



    }


  simulator->SetStop(duration);
  simulator->Schedule(duration-10, &Simulator::PrintMemoryUsage, simulator);
  simulator->Run ();

}
