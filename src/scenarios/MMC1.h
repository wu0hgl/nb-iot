/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010,2011,2012,2013 TELEMATICS LAB, Politecnico di Bari
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
#include "../flows/application/FTP2.h"
#include "../flows/application/TraceBased.h"
#include "../device/IPClassifier/ClassifierParameters.h"
#include "../flows/QoS/QoSParameters.h"
#include "../flows/QoS/QoSForEXP.h"
#include "../flows/QoS/QoSForFLS.h"
#include "../flows/QoS/QoSForM_LWDF.h"
#include "../componentManagers/FrameManager.h"
#include "../utility/RandomVariable.h"
#include "../utility/beta_distribution.hpp"
#include "../channel/propagation-model/channel-realization.h"
#include "../phy/wideband-cqi-eesm-error-model.h"
#include "../phy/simple-error-model.h"
#include "../load-parameters.h"
#include <iostream>
#include <queue>
#include <fstream>
#include <stdlib.h>
#include <cstring>
#include <random>
#include "../protocolStack/mac/random-access/ue-random-access.h"
#include "../protocolStack/mac/random-access/enb-random-access.h"


static void MMC1 (int argc, char *argv[])
{
  double radius = atof(argv[2]);
  int nbUE = atoi(argv[3]);
  int traffic_type = atoi(argv[4]);
  int sched_type = atoi(argv[5]);
  int frame_struct = atoi(argv[6]);
  int speed = atoi(argv[7]);
  double maxDelay = atof(argv[8]);
  int video_bit_rate = atoi(argv[9]);
  int gen_pck = atoi(argv[10]);
  int sync_tx = atoi(argv[11]);
  int random_access_type = atoi(argv[12]);
  int seed;
  if (argc==14)
    {
      seed = atoi(argv[13]);
    }
  else
    {
      seed = -1;
    }

  double access_window = 10; //in sec
  // define simulation times
  double duration = 20;
  double flow_duration = 20;
  UeRandomAccess::RandomAccessType m_UeRandomAccessType;
  EnbRandomAccess::RandomAccessType m_EnbRandomAccessType;
  switch (nbUE)
    {
      case 156:
        duration = 11;
        flow_duration = 11;
        break;
      case 1560:
        duration = 11;
        flow_duration = 11;
        break;
      case 15600:
        duration = 13;
        flow_duration = 13;
        break;
      case 156000:
        duration = 15;
        flow_duration = 15;
        break;
    }


  double bandwidth = 10; //MHz

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
  cout << "Duration: " << duration << " flow: " << flow_duration << endl;

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
    default:
      downlink_scheduler_type = ENodeB::DLScheduler_TYPE_PROPORTIONAL_FAIR;
      break;
    }
  // SET TYPE LTE/ENHANCEDLTE
  UeRandomAccess::RandomAccessType m_type;
  switch(random_access_type)
    {
    case 1:
      m_UeRandomAccessType = UeRandomAccess::RA_TYPE_LTE;
      m_EnbRandomAccessType = EnbRandomAccess::RA_TYPE_LTE;
      cout << "Random Access Type LTE" << endl;
      break;
    case 2:
      m_UeRandomAccessType = UeRandomAccess::RA_TYPE_ENHANCED_LTE;
      m_EnbRandomAccessType = EnbRandomAccess::RA_TYPE_ENHANCED_LTE;
      cout << "Random Access Type ENHANCED-LTE" << endl;
      break;
    default:
      cout << "Error: unknown random access type: " << random_access_type << endl;
      exit(1);
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
      frame_structure = FrameManager::FRAME_STRUCTURE_FDD;
      break;
    }
  frameManager->SetFrameStructure(FrameManager::FRAME_STRUCTURE_FDD);


  //Define Application Container
  int nbCell = 1;
  int nbCBR = 1;
  int nbFTP2 = 1;

  //CBR CBRApplication[nbCBR*nbCell*nbUE];
  CBR *CBRApplication;
  CBRApplication=new CBR[nbCBR*nbCell*nbUE];
  FTP2 *FTP2Application;
  FTP2Application=new FTP2[nbFTP2*nbCell*nbUE];

  int cbrApplication = 0;
  int ftp2Application = 0;

  int destinationPort = 101;
  int applicationID = 0;



  // CREATE CELL
  Cell *cell = new Cell (0, radius, 0.035, 0, 0);
  networkManager->GetCellContainer ()->push_back (cell);

  // CREATE CHANNELS and propagation loss model
  LteChannel *dlCh = new LteChannel ();
  LteChannel *ulCh = new LteChannel ();


  // CREATE SPECTRUM
  BandwidthManager* spectrum = new BandwidthManager (bandwidth, bandwidth, 0, 0);



  //Create ENodeB
  ENodeB* enb = new ENodeB (1, cell, 0, 0);
  enb->SetRandomAccessType(m_EnbRandomAccessType);
  enb->GetPhy ()->SetDlChannel (dlCh);
  enb->GetPhy ()->SetUlChannel (ulCh);
  enb->GetPhy ()->SetBandwidthManager (spectrum);
  ulCh->AddDevice (enb);
  enb->SetDLScheduler (downlink_scheduler_type);
  enb->SetULScheduler (ENodeB::ULScheduler_TYPE_MAXIMUM_THROUGHPUT);
  networkManager->GetENodeBContainer ()->push_back (enb);


  //Create GW
  Gateway *gw = new Gateway ();
  networkManager->GetGatewayContainer ()->push_back (gw);


  //~ std::random_device rd;
  //~ std::minstd_rand gen(rd());
  std::minstd_rand gen(seed);
  sftrabbit::beta_distribution<> beta(3, 4);

  //Create UEs
  int idUE = 2;
  //nbUE = 3;
  for (int i = 0; i < nbUE; i++)
    {
      //ue's random position
      int maxXY = radius * 1000; // in metres
      double posX = (double)rand()/RAND_MAX; posX = 0.95 *
            (((2*radius*1000)*posX) - (radius*1000));
      double posY = (double)rand()/RAND_MAX; posY = 0.95 *
            (((2*radius*1000)*posY) - (radius*1000));
      double speedDirection = (double)(rand() %360) * ((2*3.14)/360);

      UserEquipment* ue = new UserEquipment (idUE,
                                             posX, posY, speed, speedDirection,
                                             cell,
                                             enb,
                                             0, //handover false!
                                             Mobility::CONSTANT_POSITION);

      cout << "Created UE - id " << idUE << " position " << posX << " " << posY << endl;
      ue->SetRandomAccessType(m_UeRandomAccessType);
      ue->SetTimePositionUpdate (1.0);
      ue->GetPhy ()->SetDlChannel (dlCh);
      ue->GetPhy ()->SetUlChannel (ulCh);

      FullbandCqiManager *cqiManager = new FullbandCqiManager ();
      cqiManager->SetCqiReportingMode (CqiManager::PERIODIC);
      cqiManager->SetReportingInterval (1);
      cqiManager->SetDevice (ue);
      ue->SetCqiManager (cqiManager);

      WidebandCqiEesmErrorModel *errorModel = new WidebandCqiEesmErrorModel ();
      ue->GetPhy ()->SetErrorModel (errorModel);

      networkManager->GetUserEquipmentContainer ()->push_back (ue);

      // register ue to the enb
      enb->RegisterUserEquipment (ue);
      // define the channel realization
      //ChannelRealization* c_dl = new ChannelRealization (enb, ue, ChannelRealization::CHANNEL_MODEL_MACROCELL_URBAN);
      //enb->GetPhy ()->GetDlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_dl);
      ChannelRealization* c_ul = new ChannelRealization (ue, enb, ChannelRealization::CHANNEL_MODEL_MACROCELL_URBAN);
      c_ul->disableFastFading();
      enb->GetPhy ()->GetUlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_ul);


      // CREATE DOWNLINK APPLICATION FOR THIS UE
      double start_time;
      double duration_time;

      double beta_gen;
      int gen1;
      switch(sync_tx)
        {
        case 1:
          beta_gen = beta(gen)*1000*access_window;
          gen1 = beta_gen;
          start_time = 0.5 + (float)gen1/1000; //funziona beta per ogni utente e moltiplicare per 10
          duration_time = start_time + flow_duration;
          cout << "SYNCHRONIZED TRANSMISSION" << endl;
          break;
        case 2:
          start_time = 0.5 + (double) (rand() %5);
          duration_time = start_time + flow_duration;
          cout << "NOT SYNCHRONIZED TRANSMISSION" << endl;
          break;
        default:
          start_time = 0.5 + access_window * beta(gen);
          duration_time = start_time + flow_duration;
          cout << "SYNCHRONIZED TRANSMISSION" << endl;
        }


      double pck;
      switch(gen_pck)
        {
        case 1:
          pck = 300;
          cout << "CREATED PACKET IN " << pck << " s" << endl;
          break;
        case 2:
          pck = 1800;
          cout << "CREATED PACKET IN " << pck << " s" << endl;
          break;
        case 3:
          pck = 3600;
          cout << "CREATED PACKET IN " << pck << " s" << endl;
          break;
        }


      switch (traffic_type)
        {
        case 1:
          // *** cbr application
          for (int j = 0; j < nbCBR; j++)
            {
              // create application
              CBRApplication[cbrApplication].SetSource (ue); //gw
              CBRApplication[cbrApplication].SetDestination (gw); //ue
              CBRApplication[cbrApplication].SetApplicationID (applicationID);
              CBRApplication[cbrApplication].SetStartTime(start_time);
              CBRApplication[cbrApplication].SetStopTime(duration_time);
              CBRApplication[cbrApplication].SetInterval (pck);
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
          break;
        case 2:
          // *** ftp2 application
          for (int j = 0; j < nbFTP2; j++)
            {
              // create application
              FTP2Application[ftp2Application].SetSource (ue); //gw
              FTP2Application[ftp2Application].SetDestination (gw); //ue
              FTP2Application[ftp2Application].SetApplicationID (applicationID);
              FTP2Application[ftp2Application].SetStartTime(start_time);
              FTP2Application[ftp2Application].SetStopTime(duration_time);
              FTP2Application[ftp2Application].SetAverageInterval (pck);
              FTP2Application[ftp2Application].SetSize (5);

              // create qos parameters
              QoSParameters *qosParameters = new QoSParameters ();
              qosParameters->SetMaxDelay (maxDelay);

              FTP2Application[ftp2Application].SetQoSParameters (qosParameters);

              //create classifier parameters
              ClassifierParameters *cp = new ClassifierParameters (gw->GetIDNetworkNode(),
                                                                   ue->GetIDNetworkNode(),
                                                                   0,
                                                                   destinationPort,
                                                                   TransportProtocol::TRANSPORT_PROTOCOL_TYPE_UDP);
              FTP2Application[ftp2Application].SetClassifierParameters (cp);

              cout << "CREATED FTP2 APPLICATION, ID " << applicationID << endl;

              //update counter
              destinationPort++;
              applicationID++;
              ftp2Application++;
            }
        }
      idUE++;
    }

  simulator->SetStop(duration);
  //simulator->Schedule(duration-10, &Simulator::PrintMemoryUsage, simulator);
  simulator->Run ();

}
