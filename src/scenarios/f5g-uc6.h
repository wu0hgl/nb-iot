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
#include "../protocolStack/packet/Packet.h"
#include "../core/eventScheduler/simulator.h"
#include "../flows/application/InfiniteBuffer.h"
#include "../flows/application/VoIP.h"
#include "../flows/application/CBR.h"
#include "../flows/application/FTP2.h"
#include "../flows/application/TraceBased.h"
#include "../flows/application/ExternalSource.h"
#include "../device/IPClassifier/ClassifierParameters.h"
#include "../device/MulticastDestination.h"
#include "../flows/QoS/QoSParameters.h"
#include "../flows/QoS/QoSForEXP.h"
#include "../flows/QoS/QoSForFLS.h"
#include "../protocolStack/mac/harq-manager.h"
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


static void f5g_BroadcastServices (int argc, char *argv[])
{
  string environment(argv[2]); // "suburban" or "rural"
  double isd = atof(argv[3]); // in km
  int userDensity = atoi(argv[4]);
  int mbsfnPattern = atoi(argv[5]);
  double duration = atof(argv[6]);
  int mcs = atoi(argv[7]);
  int use_harq = atoi(argv[8]);
  int seed;
  if (argc==10)
    {
      seed = atoi(argv[9]);
    }
  else
    {
      seed = -1;
    }

  double minDist = 0.025;     // in km
  double bandwidth = 20;  // in MHz
  int ueRx = 8;
  int enbTx;
  int txMode;
  if (use_harq==0)
    {
      // assume simpler transmission to speed up simulation
      enbTx = 1;
      txMode = 1;
    }
  else
    {
      enbTx = 8;
      txMode = 9;
    }
  double txPower = 46; // dBm

  double pLossMean = 20; // dB - check
  double pLossStdDev = 0;
  int etilt;

  double vBeamWidth = 65;
  double antennaGain = 15; // dBi
  double avgBuildingHeight = 30; // m
  double antennaHeight = avgBuildingHeight + 15; // m
  double antennaAttenuation = 20; // dB
  double BSFeederLoss = 2; // dB
  double carrierFreq;
  int videoBitRate = 17000; // kbps
  //int videoBitRate = 440; // kbps
  double maxDelay = 0.1; // s
  int nbActiveCell = 1; // number of sites with active users
  double flow_duration = duration-1;
  bool handoverEnabled = true;
  Mobility::MobilityModel mobilityModel = Mobility::CONSTANT_POSITION; //Mobility::RANDOM_DIRECTION;

  int nbCell = 37;
  int nbSector = 3;

  bool create_be=true;
  bool create_mc=true;

  if(environment=="suburban")
    {
      etilt = 14;
      carrierFreq = 2000;
    }
  else if(environment=="rural")
    {
      etilt = 10;
      carrierFreq = 800;
    }
  else
    {
      cout << "ERROR: invalid environment selected" << endl;
      exit(EXIT_FAILURE);
    }
  //vector<double> speed_thresholds = {0.0, 0.65, 0.8, 0.95, 0.975};
  //vector<double> speed_values = {0, 30, 120, 250, 500};
  vector<double> speed_thresholds = {0.0, 0.65, 0.975};
  vector<double> speed_values = {3, 30, 120};

  // define cell radius from inter site distance
  double radius = isd / sqrt(3);

  // define channel model
  ChannelRealization::ChannelModel model = ChannelRealization::CHANNEL_MODEL_MACROCELL_RURAL;

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
      srand (time(NULL));
    }
  cout << "Simulation with SEED = " << seed << endl;

  // SET SCHEDULING ALLOCATION SCHEME
  ENodeB::DLSchedulerType downlink_scheduler_type = ENodeB::DLScheduler_TYPE_ROUND_ROBIN;

  // SET FRAME STRUCTURE
  frameManager->SetFrameStructure(FrameManager::FRAME_STRUCTURE_FDD);
  frameManager->SetMbsfnPattern(mbsfnPattern);

  //Create GW
  Gateway *gw = new Gateway ();
  networkManager->GetGatewayContainer ()->push_back (gw);

  //Define Application Container
  TraceBased MCApplication;
  int beApplication = 0;
  int destinationPort = 101;
  int applicationID = 0;

  int idUE = nbCell*nbSector;
  int idMulticast = 10000;
  MulticastDestination* mc = NULL;

  // CREATE CELLS
  vector <Cell*> *cells = new vector <Cell*>;
  for (int i = 0; i < nbCell; i++)
    {
      CartesianCoordinates center =
        GetCartesianCoordinatesForCell(i, radius * 1000.);

      Cell *c = new Cell (i, radius, minDist, center.GetCoordinateY (), center.GetCoordinateX ());
      cells->push_back (c);
      networkManager->GetCellContainer ()->push_back (c);

      cout << "Created Cell, id " << c->GetIdCell ()
                <<", position: " << c->GetCellCenterPosition ()->GetCoordinateX ()
                << " " << c->GetCellCenterPosition ()->GetCoordinateY () << endl;
    }

  for (int i = 0; i < nbCell; i++)
    {
      for (int j=0; j<nbSector; j++)
        {

          // CREATE CHANNELS and propagation loss model
          LteChannel *dlCh = new LteChannel ();
          LteChannel *dlMcCh = new LteChannel ();
          LteChannel *ulCh = new LteChannel ();


          // CREATE SPECTRUM
          BandwidthManager* spectrum = new BandwidthManager (bandwidth, bandwidth, 0, 0);



          //Create ENodeB
          ENodeB* enb = new ENodeB (i*nbSector+j, cells->at (i)/*, 0, 0*/);
          enb->GetPhy ()->SetDlChannel (dlCh);
          enb->GetPhy ()->SetDlMcChannel (dlMcCh);
          enb->GetPhy ()->SetUlChannel (ulCh);
          enb->GetPhy ()->SetNoiseFigure(5);
          enb->GetPhy ()->SetCarrierFrequency(carrierFreq);
          enb->GetPhy ()->SetTxAntennas(enbTx);
          enb->GetPhy ()->SetTxPower(txPower);
          enb->GetPhy ()->SetBandwidthManager (spectrum);
          enb->GetPhy ()->SetHeight(antennaHeight);
          // Set Antenna Parameters
          if(nbSector==1)
            {
              enb->GetPhy ()->GetAntennaParameters ()->SetType(LtePhy::AntennaParameters::ANTENNA_TYPE_OMNIDIRECTIONAL);
            }
          else if(nbSector==3)
            {
              enb->GetPhy ()->GetAntennaParameters ()->SetType(LtePhy::AntennaParameters::ANTENNA_TYPE_TRI_SECTOR);
            }
          else
            {
              cout << "Error: unsupported number of cell sectors (" << nbSector << ")" <<endl;
              exit(1);
            }
          enb->GetPhy ()->GetAntennaParameters ()->SetEtilt(etilt);
          enb->GetPhy ()->GetAntennaParameters ()->SetVerticalBeamwidth3db(vBeamWidth);
          enb->GetPhy ()->GetAntennaParameters ()->SetGain(antennaGain);
          enb->GetPhy ()->GetAntennaParameters ()->SetMaxHorizontalAttenuation(antennaAttenuation);
          enb->GetPhy ()->GetAntennaParameters ()->SetBearing(120*j);
          enb->GetPhy ()->GetAntennaParameters ()->SetFeederLoss(BSFeederLoss);
          enb->GetMacEntity ()->SetDefaultDlTxMode(txMode);
          ulCh->AddDevice (enb);
//          if (enb->GetCell()->GetIdCell() < nbActiveCell)
          if (enb->GetCell()->GetIdCell() == 0 || enb->GetCell()->GetIdCell() == 1 || enb->GetCell()->GetIdCell() == 7)
            {
              enb->SetDLScheduler (downlink_scheduler_type);
            }
          networkManager->GetENodeBContainer ()->push_back (enb);

          if (mc == NULL && create_mc)
            {
              mc = new MulticastDestination(idMulticast,cells->at (i),enb);
              mc -> SetMcs(mcs);
              enb->RegisterUserEquipment (mc);
              mc->GetTargetNodeRecord()->SetDlTxMode(1);
              mc->GetPhy ()->SetRxAntennas(ueRx);
              WidebandCqiManager *cqiManager = new WidebandCqiManager ();
              //FullbandCqiManager *cqiManager = new FullbandCqiManager ();
              cqiManager->SetCqiReportingMode (CqiManager::PERIODIC);
              cqiManager->SetReportingInterval (5);
              cqiManager->SetDevice (mc);
              mc->SetCqiManager (cqiManager);

              // create multicast application
              double start_time = 0.05;
              double duration_time = start_time + flow_duration;
              MCApplication.SetDestination (mc);
              MCApplication.SetApplicationID (idMulticast);
              MCApplication.SetStartTime(start_time);
              MCApplication.SetStopTime(duration_time);


              string video_trace ("trailer_H264_");
              //string video_trace ("foreman_H264_");
              //string video_trace ("highway_H264_");
              //string video_trace ("mobile_H264_");

              switch (videoBitRate)
                {
                  case 128:
                  {
                    MCApplication.LoadInternalTrace(&foreman_h264_128k);
//                    MCApplication.LoadInternalTrace(&highway_h264_128k);
//                    MCApplication.LoadInternalTrace(&mobile_h264_128k);
                    cout << " selected video @ 128k " << endl;
                    MCApplication.SetBurstSize(11569);
                    break;
                  }
                  case 242:
                  {
                    MCApplication.LoadInternalTrace(&foreman_h264_242k);
                    cout << " selected video @ 242k"<< endl;
                    MCApplication.SetBurstSize(14608);
                    break;
                  }
                  case 440:
                  {
                    MCApplication.LoadInternalTrace(&foreman_h264_440k);
                    cout << " selected video @ 440k"<< endl;
                    MCApplication.SetBurstSize(22563);
                    break;
                  }
                  case 8000:
                  {
                    MCApplication.LoadInternalTrace(&trailer_h264_8000k);
                    cout << " selected video @ 8000k"<< endl;
                    MCApplication.SetBurstSize(232708);
                    break;
                  }
                  case 17000:
                  {
                    MCApplication.LoadInternalTrace(&trailer_h264_17000k);
                    cout << " selected video @ 17000k"<< endl;
                    MCApplication.SetBurstSize(494505);
                    break;
                  }
                  default:
                  {
                    cout << "Error: cannot find video trace!"<< endl;
                    exit(1);
                    break;
                  }
                }

              //QoSParameters *qosParameters = new QoSParameters ();
              QoSParameters *qosParameters = new QoSForFLS ();
              qosParameters->SetMaxDelay (maxDelay);
              MCApplication.SetQoSParameters (qosParameters);

              ClassifierParameters *cp = new ClassifierParameters (gw->GetIDNetworkNode(),
                  mc->GetIDNetworkNode(),
                  0,
                  destinationPort,
                  TransportProtocol::TRANSPORT_PROTOCOL_TYPE_UDP);
              MCApplication.SetClassifierParameters (cp);

              MCApplication.SetSource (gw);

              idMulticast++;
            }
//if (enb->GetCell()->GetIdCell() < nbActiveCell && create_mc){
          if ((enb->GetCell()->GetIdCell() == 0 || enb->GetCell()->GetIdCell() == 1 || enb->GetCell()->GetIdCell() == 7) && create_mc)
            {
              enb->RegisterUserEquipment(mc);
              mc->AddSource(enb);
              MCApplication.AddTransmittingNode(enb);
            }
        }
    }


  double ue_drop_radius;
  if(nbActiveCell==1)
    {
      ue_drop_radius = isd;
    }
  else if(nbActiveCell>1 && nbActiveCell<=7)
    {
      ue_drop_radius = 2*isd;
    }
  else if(nbActiveCell>7 && nbActiveCell<=19)
    {
      ue_drop_radius = 3*isd;
    }

  double area = ue_drop_radius * ue_drop_radius * M_PI;
  int nbUE = round( area * userDensity );
  cout << "Creating " << nbUE << " user terminals" << endl;

  //Create multicast UEs
  if(create_mc)
    for (int n = 0; n < nbUE; n++)
      {
        double posX, posY;
        do
          {
            posX = ((2*ue_drop_radius*1000)*((double)rand()/RAND_MAX)) - ue_drop_radius*1000;
            posY = ((2*ue_drop_radius*1000)*((double)rand()/RAND_MAX)) - ue_drop_radius*1000;
          }
        while (sqrt(posX*posX + posY*posY)>ue_drop_radius*1000);

        int speed = speed_values.at(0);
        for(int i=0; i<(int)speed_thresholds.size(); i++)
          {
            if(n >= speed_thresholds.at(i)*nbUE)
              {
                speed = speed_values.at(i);
              }
          }
        double speedDirection = (double)(rand() %360) * ((2*M_PI)/360);

        ENodeB* enb = networkManager->GetENodeBContainer ()->at(0);

        UserEquipment* ue = new UserEquipment (idUE,
                                               posX, posY, speed, speedDirection,
                                               cells->at (0),
                                               enb,
                                               handoverEnabled,
                                               mobilityModel
                                              );

        cout << "Created UE - id " << idUE << " position " << posX << " " << posY << endl;

        Simulator::Init()->Schedule(0.005, &Mobility::SetHandover, ue->GetMobilityModel(), false);
        if(use_harq==0)
          {
            delete ue->GetMacEntity()->GetHarqManager();
            ue->GetMacEntity()->SetHarqManager(NULL);
          }
        LteChannel* dlCh = enb->GetPhy ()->GetDlChannel ();
        LteChannel* dlMcCh = enb->GetPhy ()->GetDlMcChannel ();
        LteChannel* ulCh = enb->GetPhy ()->GetUlChannel ();

        ue->GetPhy ()->SetDlChannel (dlCh);
        dlCh->AddDevice(ue);
        ue->GetPhy ()->SetDlMcChannel (dlMcCh);
        dlMcCh->AddDevice(ue);
        ue->GetPhy ()->SetUlChannel (ulCh);
        ulCh->AddDevice(ue);

        ue->GetPhy ()->SetNoiseFigure(9);
        ue->GetPhy ()->SetRxAntennas(ueRx);

        WidebandCqiManager *cqiManager = new WidebandCqiManager ();
        //FullbandCqiManager *cqiManager = new FullbandCqiManager ();
        cqiManager->SetCqiReportingMode (CqiManager::PERIODIC);
        cqiManager->SetReportingInterval (5);
        cqiManager->SetDevice (ue);
        ue->SetCqiManager (cqiManager);

        WidebandCqiEesmErrorModel *errorModel = new WidebandCqiEesmErrorModel ();
        ue->GetPhy ()->SetErrorModel (errorModel);

        networkManager->GetUserEquipmentContainer ()->push_back (ue);
        ue->SetMulticastDestination (mc);

        mc->AddDestination(ue);
        // register ue to the enb
        enb->RegisterUserEquipment (ue);


        //ExternalSource Creation
        ExternalSource* es = new ExternalSource();
        int ExternalSourceID = 5;
        double start_time = 0.01;
        es->SetSource (gw);
        es->SetDestination(ue);
        es->SetApplicationID (ExternalSourceID);
        es->SetStartTime(start_time);

        // define the channel realizations
        ENodeB* prevEnb = NULL;
        ChannelRealization *prev_c_dl, *prev_c_ul, *prev_c_mcdl;
        for ( auto enb : *networkManager->GetENodeBContainer () )
          {
            ChannelRealization* c_dl = new ChannelRealization (enb, ue, model, false);
            enb->GetPhy ()->GetDlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_dl);
            ChannelRealization* c_mcdl = new ChannelRealization (enb, ue, model, true);
            enb->GetPhy ()->GetDlMcChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_mcdl);
            ChannelRealization* c_ul = new ChannelRealization (ue, enb, model, false);
            enb->GetPhy ()->GetUlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_ul);

            c_dl->SetPenetrationLossMean(pLossMean);
            c_dl->SetPenetrationLossStdDev(pLossStdDev);
            c_mcdl->SetPenetrationLossMean(pLossMean);
            c_mcdl->SetPenetrationLossStdDev(pLossStdDev);
            c_ul->SetPenetrationLossMean(pLossMean);
            c_ul->SetPenetrationLossStdDev(pLossStdDev);

            // syncronize LoS state and shadowing for eNBs at the same site
            if (prevEnb != NULL && enb->GetCell()->GetIdCell() == prevEnb->GetCell()->GetIdCell())
              {
                c_dl->SetLoSState(prev_c_dl->GetLoSState());
                c_mcdl->SetLoSState(prev_c_mcdl->GetLoSState());
                c_ul->SetLoSState(prev_c_ul->GetLoSState());
                c_dl->SetShadowing(prev_c_dl->GetShadowing());
                c_mcdl->SetShadowing(prev_c_mcdl->GetShadowing());
                c_ul->SetShadowing(prev_c_ul->GetShadowing());
              }
            prevEnb = enb;
            prev_c_dl = c_dl;
            prev_c_mcdl = c_mcdl;
            prev_c_ul = c_ul;
          }

        idUE++;

      }


  simulator->SetStop(duration);
  simulator->Run ();

}

