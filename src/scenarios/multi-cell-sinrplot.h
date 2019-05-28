/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Communications Research Laboratory, Ilmenau University of Technology
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
 * Author: Florian Schlembach <florian.schlembach@tu-ilmenau.de>
 * Author2: Jianshu Zhang <jianshu.zhang@tu-ilmenau.de>
 * Author3: Yao Cheng <y.cheng@tu-ilmenau.de>
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
#include "../load-parameters.h"
#include <iostream>
#include <queue>
#include <fstream>
#include <stdlib.h>
#include <cstring>

static void MultiCellSinrPlot (int argc, char *argv[])

{
  int nbCell = atoi(argv[2]);
  double radius = atof(argv[3]);
  int nbUE = atoi(argv[4]);
  int sched_type = atoi(argv[5]);
  int frame_struct = atoi(argv[6]);
  int speed = atoi(argv[7]);
  int mobility_model = atoi(argv[8]);
  int seed;
  if (argc==10)
    {
      seed = atoi(argv[9]);
    }
  else
    {
      seed = -1;
    }

  int handover;
  if(nbCell>1) handover = 1; //if multicell scenario -> activate HO mechanisms

  Mobility::MobilityModel model;
  switch(mobility_model)
    {
    case 0:
      model = Mobility::RANDOM_DIRECTION;
      break;
    case 1:
      model = Mobility::RANDOM_WALK;
      break;
    case 2:
      model = Mobility::MANHATTAN;
      break;
    default:
      model = Mobility::RANDOM_DIRECTION;
    }


  // define simulation times
  double duration = 500;
//  double flow_duration = 1;

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




  //create cells
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


  vector <BandwidthManager*> spectrums = RunFrequencyReuseTechniques (nbCell, cluster, bandwidth);

  //Create a set of a couple of channels
  vector <LteChannel*> *dlChannels = new vector <LteChannel*>;
  vector <LteChannel*> *ulChannels = new vector <LteChannel*>;
  for (int i= 0; i < nbCell; i++)
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

  //Create GW
  Gateway *gw = new Gateway ();
  nm->GetGatewayContainer ()->push_back (gw);


  //nbUE is the number of users of the whole multicell scenario
  //Create UEs
  for (int i = 0; i < nbUE; i++)
    {
      //ue's placed in the origin at the beginning
      double posX = 0;
      double posY = 0;
      int idUE = i; //place in first cell
      double speedDirection = (double)(rand() %360) * ((2*M_PI)/360);;

      UserEquipment* ue = new UserEquipment (idUE,
                                             posX, posY, speed, speedDirection,
                                             cells->at (0),
                                             eNBs->at (0),
                                             handover,
                                             model);

      cout << "Created UE - id " << idUE << " position " << posX << " " << posY
                << ", cell " <<  ue->GetCell ()->GetIdCell ()
                << ", target enb " << ue->GetTargetNode ()->GetIDNetworkNode () << endl;

      ue->GetPhy ()->SetDlChannel (eNBs->at (0)->GetPhy ()->GetDlChannel ());
      ue->GetPhy ()->SetUlChannel (eNBs->at (0)->GetPhy ()->GetUlChannel ());

      FullbandCqiManager *cqiManager = new FullbandCqiManager ();
      cqiManager->SetCqiReportingMode (CqiManager::PERIODIC);
      cqiManager->SetReportingInterval (1);
      cqiManager->SetDevice (ue);
      ue->SetCqiManager (cqiManager);
      nm->GetUserEquipmentContainer ()->push_back (ue);

      // register ue to the enb
      eNBs->at (0)->RegisterUserEquipment (ue);
      // define the channel realization
      ChannelRealization* c_dl = new ChannelRealization (eNBs->at (0), ue, ChannelRealization::CHANNEL_MODEL_MACROCELL_URBAN);
      eNBs->at (0)->GetPhy ()->GetDlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_dl);
      ChannelRealization* c_ul = new ChannelRealization (ue, eNBs->at (0), ChannelRealization::CHANNEL_MODEL_MACROCELL_URBAN);
      eNBs->at (0)->GetPhy ()->GetUlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_ul);
      //Force attachment of the UE to the eNB
      eNBs->at (0)->GetPhy ()->GetDlChannel ()->AddDevice(ue);

      idUE++;

    }

  simulator->SetStop(duration);
  //simulator->Schedule(duration-10, &Simulator::PrintMemoryUsage, simulator);
  simulator->Run ();


}
