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
 * Author3: Emilia Salzarulo <salzarulo.e@gmail.com>
 * Author4: Felice Paparusso <felice.paparusso@gmail.com>
 * Author5: Vincenzo Fortunato <vincfort92@hotmail.it>
 */


#include "channel-realization.h"
#include "../../device/UserEquipment.h"
#include "../../device/ENodeB.h"
#include "../../device/HeNodeB.h"
#include "../../utility/RandomVariable.h"
#include "../../utility/GaussianRandomVariable.h"
#include "../../utility/IndoorScenarios.h"
#include "../../core/spectrum/bandwidth-manager.h"
#include "../../phy/lte-phy.h"
#include "../../phy/enb-lte-phy.h"
#include "../../core/eventScheduler/simulator.h"
#include "../../load-parameters.h"
#include "fast-fading-trace.h"
#include "../../componentManagers/FrameManager.h"
#include <assert.h>
#include <complex>
#include <string.h>


ChannelRealization::ChannelRealization (NetworkNode* src, NetworkNode* dst, ChannelModel model, bool MBSFNRealization)
{
  m_samplingPeriod = 0.5;
  m_lastUpdate = -INFINITY;
  m_shadowing = 0;
  m_indoorDistance = 0;
  m_isLosType = true;
  m_lastShadowingUpdate = 0;
  m_lastPenetrationLossUpdate = 0;
  m_mbsfnRealization = MBSFNRealization;
  m_lastShortTermUpdate = -INFINITY;

  m_forceCoverShift = -1;

  switch(model)
    {
      case CHANNEL_MODEL_MACROCELL_URBAN:

      case CHANNEL_MODEL_MACROCELL_SUB_URBAN:
      case CHANNEL_MODEL_MACROCELL_RURAL:
      case CHANNEL_MODEL_MICROCELL:
      case CHANNEL_MODEL_3GPP_DOWNLINK:
        m_penetrationLossMean = 10;
        m_penetrationLossStdDev = 0;
        break;

      case CHANNEL_MODEL_WINNER_DOWNLINK:
      case CHANNEL_MODEL_FEMTOCELL_URBAN:
        m_penetrationLossMean = 0;
        m_penetrationLossStdDev = 0;
        break;

      case CHANNEL_MODEL_MACROCELL_URBAN_IMT:
      case CHANNEL_MODEL_MACROCELL_RURAL_IMT:
        m_penetrationLossMean = 9;
        m_penetrationLossStdDev = 5;
        break;

      case CHANNEL_MODEL_MACROCELL_URBAN_IMT_3D:
        m_penetrationLossMean = 0;
        m_penetrationLossStdDev = 0;

      case CHANNEL_MODEL_3GPP_CASE1:
        m_penetrationLossMean = 20;
        m_penetrationLossStdDev = 0;
        break;

    }
  m_channelModel = model;

  m_pathLoss = 0;
  m_penetrationLoss = GetGaussianRandomVariable(m_penetrationLossMean, m_penetrationLossStdDev);

  SetSourceNode (src);
  SetDestinationNode (dst);

  UserEquipment* ue;
  ENodeB* enb;

  if (src->GetNodeType () == NetworkNode::TYPE_UE
        && ( dst->GetNodeType () == NetworkNode::TYPE_ENODEB
              || dst->GetNodeType () == NetworkNode::TYPE_HOME_BASE_STATION) )
    {
      ue = (UserEquipment*) src;
      enb = (ENodeB*) dst;
    }
  else if (dst->GetNodeType () == NetworkNode::TYPE_UE
            && ( src->GetNodeType () == NetworkNode::TYPE_ENODEB
                  || src->GetNodeType () == NetworkNode::TYPE_HOME_BASE_STATION) )
    {
      ue = (UserEquipment*) dst;
      enb = (ENodeB*) src;
    }
  else
    {
      cout << "Error: invalid source/destination pair for channel realization (must be 1 ue and 1 enb)" << endl;
      exit(1);
    }


DEBUG_LOG_START_1(LTE_SIM_TEST_PROPAGATION_LOSS_MODEL)
  cout << "Created Channe Realization between "
        << src->GetIDNetworkNode () << " and " << dst->GetIDNetworkNode () << endl;
DEBUG_LOG_END

    double f = src->GetPhy ()->GetCarrierFrequency ();
    double losProbability, randomProb;
    CartesianCoordinates* uePos = ue->GetMobilityModel()->GetAbsolutePosition();
    CartesianCoordinates* enbPos = enb->GetMobilityModel()->GetWrapAroundPosition(uePos);
    double distance = uePos->GetDistance (enbPos);

    switch (m_channelModel){

        case CHANNEL_MODEL_MACROCELL_URBAN_IMT:
            losProbability = min(18/distance, 1.0) * (1 - exp(-distance/63)) + exp(-distance/63);
            randomProb = ((double) rand() / RAND_MAX);

            if (randomProb <= losProbability){
                m_isLosType = true;
                m_shadowingStddev = 4;
            } else {
                m_isLosType = false;
                m_shadowingStddev = 6;
            }
            break;


        case CHANNEL_MODEL_MACROCELL_URBAN_IMT_3D: // TODO use correct los formula, see [36.873]
            losProbability = min(18/distance, 1.0) * (1 - exp(-distance/63)) + exp(-distance/63);
            randomProb = ((double) rand() / RAND_MAX);

            if (randomProb <= losProbability)
              {
                m_isLosType = true;
                m_shadowingStddev = 4;
              }
            else
              {
                m_isLosType = false;
                m_shadowingStddev = 6;
              }

            if (ue->IsIndoor())
              {
                m_penetrationLossMean = 20;
                m_shadowingStddev = 7;
                m_indoorDistance = (double)rand()/RAND_MAX*25;
              }
            break;

        case CHANNEL_MODEL_MACROCELL_RURAL_IMT:
            losProbability = min(1.00, exp(-(distance-10)/1000));
            randomProb = ((double) rand() / RAND_MAX);

            if (randomProb <= losProbability){
                m_isLosType = true;
                double dbp = 2*M_PI * 35 * 1.5 * (f/300); // 2pi * hBS * hUT * (f/c)
                if (distance >10 && distance < dbp){
                    m_shadowingStddev = 4;
                } else if (distance > dbp && distance < 10000){
                    m_shadowingStddev = 6;
                }
            } else {
                m_isLosType = false;
                m_shadowingStddev = 8;
            }
            break;

        case CHANNEL_MODEL_3GPP_CASE1:
            m_shadowingStddev = 8;
            break;

        default:
            m_shadowingStddev = 8;
            break;
        }
      m_shadowing = GetGaussianRandomVariable(0,m_shadowingStddev);


  m_fastFading = nullptr;
  if (ue->GetTargetNode()==enb || isMbsfnRealization() == true)
    {
      enableFastFading();
    }
}


ChannelRealization::~ChannelRealization()
{
  Destroy ();
}

void
ChannelRealization::Destroy ()
{
  if (m_fastFading != nullptr)
    {
      for (auto f : *m_fastFading)
        {
          f.clear();
        }
      m_fastFading->clear ();
      delete m_fastFading;
    }
  m_src = nullptr;
  m_dst = nullptr;
}

void
ChannelRealization::SetSourceNode (NetworkNode* src)
{
  m_src = src;
}

NetworkNode*
ChannelRealization::GetSourceNode (void)
{
  return m_src;
}

void
ChannelRealization::SetDestinationNode (NetworkNode* dst)
{
  m_dst = dst;
}

NetworkNode*
ChannelRealization::GetDestinationNode (void)
{
  return m_dst;
}

void
ChannelRealization::SetLastUpdate (void)
{
  m_lastUpdate = Simulator::Init()->Now ();
}


double
ChannelRealization::GetLastUpdate (void)
{
  return m_lastUpdate;
}


bool
ChannelRealization::hasFastFading()
{
  if (m_fastFading == nullptr)
    {
      return false;
    }
  else
    {
      return true;
    }
}


void
ChannelRealization::enableFastFading()
{
  if (hasFastFading()==true)
    {
      return;
    }

  // number of antennas for MIMO configurations
  int M;
  if (m_mbsfnRealization == true)
    {
      M = 1;
    }
  else
    {
      M = m_src->GetPhy()->GetTxAntennas();
    }
  int N = m_dst->GetPhy()->GetRxAntennas();
  int numbOfSubChannels = GetSourceNode ()->GetPhy ()->GetBandwidthManager ()->GetDlSubChannels ().size ();

  SetFastFading (new FastFading ());
  m_fastFading->resize(M*N);
  m_fastFadingPhases = new FastFading ();
  m_fastFadingPhases->resize(M*N);

  UpdateFastFading ();
}


void
ChannelRealization::disableFastFading()
{
  delete m_fastFading;
  m_fastFading = nullptr;

  delete m_fastFadingPhases;
  m_fastFadingPhases = nullptr;
}


void
ChannelRealization::ShortTermUpdate(void)
{
  if(Simulator::Init()->Now() > m_lastShortTermUpdate) {

    NetworkNode* src = GetSourceNode ();
    NetworkNode* dst = GetDestinationNode ();

    UserEquipment* ue;
    ENodeB* enb;

    if (src->GetNodeType () == NetworkNode::TYPE_UE
        && ( dst->GetNodeType () == NetworkNode::TYPE_ENODEB
              || dst->GetNodeType () == NetworkNode::TYPE_HOME_BASE_STATION) )
      {
        ue = (UserEquipment*) src;
        enb = (ENodeB*) dst;
      }
    else if (dst->GetNodeType () == NetworkNode::TYPE_UE
            && ( src->GetNodeType () == NetworkNode::TYPE_ENODEB
                  || src->GetNodeType () == NetworkNode::TYPE_HOME_BASE_STATION) )
      {
        ue = (UserEquipment*) dst;
        enb = (ENodeB*) src;
      }

    double f = src->GetPhy ()->GetCarrierFrequency ();
    double losProbability, randomProb;
    CartesianCoordinates* uePos = ue->GetMobilityModel()->GetAbsolutePosition();
    CartesianCoordinates* enbPos = enb->GetMobilityModel()->GetWrapAroundPosition(uePos);
    double distance = uePos->GetDistance (enbPos);
    double shadowing_correlation_distance;

    double speed = ue->GetMobilityModel()->GetSpeed () / 3.6; //convert to m/s

    switch (m_channelModel)
      {

      case CHANNEL_MODEL_MACROCELL_URBAN_IMT:
        if (m_isLosType)
          {
            shadowing_correlation_distance = 37;
            m_shadowingStddev = 4;
          }
        else
          {
            shadowing_correlation_distance = 50;
            m_shadowingStddev = 6;
          }
        break;

      case CHANNEL_MODEL_MACROCELL_URBAN_IMT_3D:
        if(ue->IsIndoor() == true)
          {
            shadowing_correlation_distance = 7;
            m_shadowingStddev = 7;
          }
        else if (m_isLosType)
          {
            shadowing_correlation_distance = 37;
            m_shadowingStddev = 4;
          }
        else
          {
            shadowing_correlation_distance = 50;
            m_shadowingStddev = 6;
          }
        break;

      case CHANNEL_MODEL_MACROCELL_RURAL_IMT:

        if (m_isLosType)
          {
            double dbp = 2*M_PI * 35 * 1.5 * (f/300); // 2pi * hBS * hUT * (f/c)
            if (distance < dbp)
              {
                m_shadowingStddev = 4;
              }
            else
              {
                m_shadowingStddev = 6;
              }
            shadowing_correlation_distance = 37;
          }
        else
          {
            shadowing_correlation_distance = 120;
            m_shadowingStddev = 8;
          }
        break;

      case CHANNEL_MODEL_3GPP_CASE1:
        shadowing_correlation_distance = 120;
        m_shadowingStddev = 8;
        break;

      default:
        shadowing_correlation_distance = 120;
        m_shadowingStddev = 8;
        break;
      }

    double shadowing_correlation_factor = exp(-(Simulator::Init()->Now()-m_lastShortTermUpdate)*speed/shadowing_correlation_distance);
    double e = GetGaussianRandomVariable(0,m_shadowingStddev*sqrt(1-pow(shadowing_correlation_factor,2)));
    m_shadowing = m_shadowing * shadowing_correlation_factor + e;

    m_lastShortTermUpdate = Simulator::Init()->Now();
  }
}

void
ChannelRealization::SetSamplingPeriod (double sp)
{
  m_samplingPeriod = sp;
}


double
ChannelRealization::GetSamplingPeriod (void)
{
  return m_samplingPeriod;
}

double
ChannelRealization::GetPathLoss (void)
{
  ShortTermUpdate();
  NetworkNode* src = GetSourceNode ();
  NetworkNode* dst = GetDestinationNode ();

  UserEquipment* ue;
  ENodeB* enb;

  if (src->GetNodeType () == NetworkNode::TYPE_UE
        && ( dst->GetNodeType () == NetworkNode::TYPE_ENODEB
              || dst->GetNodeType () == NetworkNode::TYPE_HOME_BASE_STATION) )
    {
      ue = (UserEquipment*) src;
      enb = (ENodeB*) dst;
    }
  else if (dst->GetNodeType () == NetworkNode::TYPE_UE
            && ( src->GetNodeType () == NetworkNode::TYPE_ENODEB
                  || src->GetNodeType () == NetworkNode::TYPE_HOME_BASE_STATION) )
    {
      ue = (UserEquipment*) dst;
      enb = (ENodeB*) src;
    }
  CartesianCoordinates* uePos = ue->GetMobilityModel()->GetAbsolutePosition();
  CartesianCoordinates* enbPos = enb->GetMobilityModel()->GetWrapAroundPosition(uePos);
  double distance = uePos->GetDistance (enbPos);
  double distance3D = uePos->GetDistance3D (enbPos);

  double Henb = enbPos->GetCoordinateZ ();
  double Hb = enb->GetPhy ()->GetAverageBuildingHeight ();
  double Hue = uePos->GetCoordinateZ ();
  double f = src->GetPhy ()->GetCarrierFrequency ();

  // used in CHANNEL_MODEL_MACROCELL_URBAN and CHANNEL_MODEL_WINNER_DOWNLINK
  double ExternalWallsAttenuation;
  // used in CHANNEL_MODEL_FEMTOCELL_URBAN
  double floorPenetration;
  double minimumCouplingLoss;
  // used in CHANNEL_MODEL_WINNER_DOWNLINK
  double InternalWallsAttenuation;
  // used in CHANNEL_MODEL_URBAN_IMT and CHANNEL_MODEL_RURAL_IMT
  double losProbability;
  double dbp, dbp1;
  double randomProb;
  //srand(time(nullptr));
  double A, B, C;
  int* nbWalls;

  switch(m_channelModel)
    {
      case CHANNEL_MODEL_MACROCELL_URBAN:
        /*
         * According to 3GPP TR 36.942
         * the Path Loss Model For Urban Environment is
         * L =  80dB + 40 ⋅ (1 − 4 ⋅ 10 − 3 ⋅ Dhb) ⋅ log 10 (R) − 18 ⋅ log 10 (Dhb) + 21 ⋅ log 10 (f)
         * R, in kilometers, is the distance between two nodes
         * Dhb, in meters, is the height of the base station above rooftop level
         * f, in MHz, is the carrier frequency
         */
        m_pathLoss = 80 + 40*( 1 - 4 * 0.001 * (Henb-Hb) )*log10(distance * 0.001) - 18*log10(Henb-Hb) + 21*log10(f);
        if ( ue->IsIndoor() )
          {
            ExternalWallsAttenuation = 20; //[dB]
            m_pathLoss = m_pathLoss + ExternalWallsAttenuation;
          }
        break;

      case CHANNEL_MODEL_3GPP_CASE1:
      case CHANNEL_MODEL_MACROCELL_SUB_URBAN :
        /*
         * According to  ---  insert standard 3gpp ---
         * the Path Loss Model For Urban Environment is
         * L = I + 37.6log10(R)
         * R, in kilometers, is the distance between two nodes
         * I = 128.1 at 2GHz
         */
        m_pathLoss = 128.1 + (37.6 * log10 (distance * 0.001));
        break;


      case CHANNEL_MODEL_MACROCELL_RURAL:
        /*
         * According to  ---  insert standard 3gpp ---
         * the Path Loss Model For Rural Environment is
         * L (R)= 69.55 +26.16log 10 (f)–13.82log 10 (Hb)+[44.9-6.55log 10 (Hb)]log(R) – 4.78(Log 10 (f)) 2 +18.33 log 10 (f) -40.94
         * R, in kilometers, is the distance between two nodes
         * f, in MHz, is the carrier frequency
         * Hb, in meters, is the base station antenna height above ground
         */
        m_pathLoss = 69.55 + 26.16*log10(f) - 13.82*log10(Henb) + (44.9-6.55*log10(Henb))*log10(distance * 0.001) - 4.78*pow(log10(f),2) + 18.33*log10(f) - 40.94;
        break;


      case CHANNEL_MODEL_MICROCELL:
        /*
         * According to  ---  insert standard 3gpp ---
         * the Path Loss Model For Rural Environment is
         * L = 24 + 345log10(R)   * R, in meters, is the distance between two nodes
         */
        m_pathLoss = 24 + (45 * log10 (distance));
        break;


      case CHANNEL_MODEL_FEMTOCELL_URBAN:
        /*
         * Path loss Models from sect. 5.2 in
         * 3GPP TSG RAN WG4 R4-092042
         *
         * Alternative simplified model based on LTE-A evaluation methodology which avoids modeling any walls.
         */

        minimumCouplingLoss = 45; //[dB] - see 3GPP TSG RAN WG4 #42bis (R4-070456)
        floorPenetration = 0.0;
        //18.3 n ((n+2)/(n+1)-0.46)

        if( enb->GetCell()->GetCellCenterPosition()->GetFloor() > 0
            && ue->IsIndoor()
            && enb->GetCell()->GetCellCenterPosition()->GetFloor() != ue->GetCell()->GetCellCenterPosition()->GetFloor())
          {
            int n = (int) abs( enb->GetCell()->GetCellCenterPosition()->GetFloor() - ue->GetCell()->GetCellCenterPosition()->GetFloor() );
            floorPenetration = 18.3 * pow( n, ((n+2)/(n+1)-0.46));
          }
        m_pathLoss = max( minimumCouplingLoss, 127 + ( 30 * log10 (distance * 0.001) ) + floorPenetration);
        break;


      case CHANNEL_MODEL_3GPP_DOWNLINK:
        /*
         * Path Loss Model For Indoor Environment.
         * L = 37 + 30 Log10(R) , R in meters
         * at the same floor
         */
        m_pathLoss = 37 + (30 * log10 (distance));
        break;


      case CHANNEL_MODEL_WINNER_DOWNLINK:
        /*
         * Path Loss Model For Indoor Environment.
         * "WINNER II channel models, ver 1.1, Tech Report"
         * PL = A*log10(r) + B + C*log10(fc/5) + X; [r in meters; fc in GHz]
         * I = 128.1 – 2GHz
         * X depends on the number of walls in between
         * FL = 17 + 4 (Nfloors - 1) --- floor loss
         */
        assert (dst->GetNodeType () == NetworkNode::TYPE_HOME_BASE_STATION  || src->GetNodeType () == NetworkNode::TYPE_HOME_BASE_STATION);

        nbWalls = GetWalls( (Femtocell*) (enb->GetCell()), ue);

        ExternalWallsAttenuation = 20.0;
        InternalWallsAttenuation = 10.0;

        if (nbWalls[0] == 0 && nbWalls[1] == 0)
          { //LOS
            A = 18.7;
            B = 46.8;
            C = 20.0;
          }
        else
          { //NLOS
            A = 20.0;
            B = 46.4;
            C = 20.0;
          }

        m_pathLoss = A * log10( distance ) +
                             B +
                             C * log10(2. / 5.0) +
                             InternalWallsAttenuation * nbWalls[1] +
                             ExternalWallsAttenuation * nbWalls[0];

        delete [] nbWalls;
        break;


      case CHANNEL_MODEL_MACROCELL_URBAN_IMT:
        dbp1 = 4*(Henb-1)*(Hue-1)*(f/300); // 4*(h'BS)*(h'UT)*(f/c)
        if(m_isLosType)
          {
            if(distance < dbp1)
              {
                m_pathLoss = 22.0*log10(distance) + 28 + 20*log10(f*0.001);
              }
            else
              {
                m_pathLoss = 40*log10(distance) + 7.8 - 18*log10(Henb-1)-18*log10(Hue-1)+2*log10(f*0.001);
              }
          }
        else
          {
            m_pathLoss = 161.04 - 7.1*log10(20) + 7.5*log10(Hb) - (24.37 - 3.7*pow(Hb/Henb, 2)) * log10(Henb) + (43.42 - 3.1*log10(Henb)) * (log10(distance) - 3) + 20*log10(f * 0.001) - (3.2 * pow(log10(11.75*Hue), 2) - 4.97);
          }
        break;


      case CHANNEL_MODEL_MACROCELL_URBAN_IMT_3D:
        dbp1 = 4*(Henb-1)*(Hue-1)*(f/300); // 4*(h'BS)*(h'UT)*(f/c)

        if(m_isLosType)
          {
            if(distance < dbp1)
              {
                m_pathLoss = 22.0*log10(distance3D) + 28 + 20*log10(f*0.001);
              }
            else
              {
                m_pathLoss = 40*log10(distance3D) + 28 + 20*log10(f*0.001) - 9*log10(pow(dbp1,2)+pow(Henb-Hue,2));
              }
          }
        else
          {
            double W = 20; // street width in meters
            m_pathLoss = 161.04 - 7.1*log10(W) + 7.5*log10(Hb) - (24.37 - 3.7*pow(Hb/Henb, 2)) * log10(Henb) + (43.42 - 3.1*log10(Henb)) * (log10(distance3D) - 3) + 20*log10(f * 0.001) - (3.2 * pow(log10(17.625), 2) - 4.97) - 0.6*(Hue-1.5);
          }

        if(ue->IsIndoor())
          {
            m_pathLoss += 0.5 * m_indoorDistance;
          }
        break;


      case CHANNEL_MODEL_MACROCELL_RURAL_IMT:
      dbp = 2*M_PI * Henb * 1.5 * (f/300); // 2pi * hBS * hUT * (f/c)
      if(m_isLosType)
        {
          if(distance < dbp)
            {
              m_pathLoss = 20*log10(40*M_PI*distance*(f*0.001/3)) + min(0.03*pow(Hb, 1.72), 10.00)*log10(distance) - min(0.044*pow(Hb, 1.72), 14.77)+0.002*log10(Hb)*distance;
            }
          else
            {
              m_pathLoss = 20*log10(40*M_PI*dbp*(f*0.001/3)) + min(0.03*pow(Hb, 1.72), 10.00)*log10(dbp) - min(0.044*pow(Hb, 1.72), 14.77)+0.002*log10(Hb)*dbp + (40*log10(distance/dbp));
            }
        }
      else
        {
          m_pathLoss = 161.04 - 7.1*log10(20) + 7.5*log10(Hb) - (24.37 - 3.7*pow(Hb/Henb, 2)) * log10(Henb) + (43.42 - 3.1*log10(Henb)) * (log10(distance) - 3) + 20*log10(f * 0.001) - (3.2 * pow(log10(11.75*1.5), 2) - 4.97);
        }
      break;


  }

  if( enb->GetPhy ()->GetAntennaParameters ()->GetType() == LtePhy::AntennaParameters::ANTENNA_TYPE_TRI_SECTOR )
    {
      double enb_bearing = enb->GetPhy ()->GetAntennaParameters ()->GetBearing ();
      double ue_bearing = uePos->GetPolarAzimut (enbPos) * 57.2957795;
      double relative_angle = enb_bearing-ue_bearing;

      // adjust to [-180°, +180]
      while (relative_angle >  180 ) {relative_angle -= 360;}
      while (relative_angle < -180 ) {relative_angle += 360;}

      double antennaGain = enb->GetPhy ()->GetAntennaParameters ()->GetGain ();
      double max_attenuation_h = enb->GetPhy ()->GetAntennaParameters ()->GetMaxHorizontalAttenuation ();
      double beamwidth_3dB_h = enb->GetPhy ()->GetAntennaParameters ()->GetHorizontalBeamwidth3db ();
      double directional_loss_h = +min( 12*pow(relative_angle/beamwidth_3dB_h,2),max_attenuation_h);
      double relative_angle_v = atan(
                                  (enbPos->GetCoordinateZ () - uePos->GetCoordinateZ ()) /
                                  uePos->GetDistance(enbPos)
                                ) * 57.2957795;
      double max_attenuation_v = enb->GetPhy ()->GetAntennaParameters ()->GetMaxVerticalAttenuation ();
      double beamwidth_3dB_v = enb->GetPhy ()->GetAntennaParameters ()->GetVerticalBeamwidth3db ();
      double etilt = enb->GetPhy()->GetAntennaParameters ()->GetEtilt();
      double directional_loss_v = +min(12*pow((relative_angle_v-etilt)/beamwidth_3dB_v,2), max_attenuation_v);
      double directional_loss;
      if (beamwidth_3dB_v == 0)
        {
          directional_loss = directional_loss_h;
        }
      else
        {
          directional_loss = min((directional_loss_h+directional_loss_v), max_attenuation_h);
        }
      m_pathLoss += - antennaGain + directional_loss;

DEBUG_LOG_START_1(LTE_SIM_TRIPLE_SECTOR_DEBUG)
      cout << "ID UE: " << ue->GetIDNetworkNode()
        << ",\tID ENB: " << enb->GetIDNetworkNode()
        << ",\tX: " << uePos->GetCoordinateX()
        << ",\tY: " << uePos->GetCoordinateY()
        << ",\tAngle " << relative_angle
        << ",\tLoss: " << directional_loss
        << endl;
DEBUG_LOG_END
    }

  double feederLoss = src->GetPhy()->GetAntennaParameters()->GetFeederLoss();

  return m_pathLoss + feederLoss;
}

void
ChannelRealization::SetShadowing (double sh)
{
  m_shadowing = sh;
}

double
ChannelRealization::GetShadowing (void)
{
  ShortTermUpdate();
  return m_shadowing;
}

void
ChannelRealization::SetShadowingStddev (double stddev)
{
  m_shadowingStddev = stddev;
}

double
ChannelRealization::GetShadowingStddev (void)
{
  return m_shadowingStddev;
}

void
ChannelRealization::SetPenetrationLoss (double pnl)
{
  m_penetrationLoss = pnl;
}

double ChannelRealization::GetPenetrationLoss (void)
{
  //if(Simulator::Init()->Now() > m_lastPenetrationLossUpdate) {
  //  m_penetrationLoss = GetGaussianRandomVariable(m_penetrationLossMean, m_penetrationLossStdDev);
  //  m_lastPenetrationLossUpdate = Simulator::Init()->Now();
  //}
  return m_penetrationLoss;
}


double ChannelRealization::GetBeamformingGain_cover (int path, int cover)
{
  m_forceCoverShift = cover;
  double result = GetBeamformingGain (path);
  m_forceCoverShift = -1;
  return result;
}


double ChannelRealization::GetBeamformingGain (int path)
{

  UserEquipment* ue;
  ENodeB* enb;

  if (m_src->GetNodeType () == NetworkNode::TYPE_UE
        && ( m_dst->GetNodeType () == NetworkNode::TYPE_ENODEB
              || m_dst->GetNodeType () == NetworkNode::TYPE_HOME_BASE_STATION) )
    {
      ue = (UserEquipment*) m_src;
      enb = (ENodeB*) m_dst;
    }
  else if (m_dst->GetNodeType () == NetworkNode::TYPE_UE
            && ( m_src->GetNodeType () == NetworkNode::TYPE_ENODEB
                  || m_src->GetNodeType () == NetworkNode::TYPE_HOME_BASE_STATION) )
    {
      ue = (UserEquipment*) m_dst;
      enb = (ENodeB*) m_src;
    }
  int txMode = ue->GetTargetNodeRecord()->GetDlTxMode();
  bool use_GoB = false;
  if (txMode == 11)
    {
      use_GoB = true;
    }

  double gain;

  if(use_GoB && m_src->GetNodeType()==NetworkNode::TYPE_ENODEB)
    {
      int M = enb->GetPhy ()->GetTxAntennas ();
      int N = ue->GetPhy ()->GetRxAntennas ();

      int num_beams;
      int beam_index;


      /*
       * The /2 factor comes from the assumption of using couples of cross-polarized antenna elements.
       * Remove if you want to assume an array of equally polarized elements.
       */
      num_beams = enb->GetPhy ()->GetTxAntennas ()/2;
      beam_index = path % (M/2);


      int beam_antennas_h = 16; // number of antennas to create the beam
      int beam_antennas_v = 8; // number of antennas to create the beam
      double f = enb->GetPhy ()->GetCarrierFrequency () * pow (10,6);;
      float const speedOfLight = 3 * pow (10,8);
      double lambda = speedOfLight / f;
      double antenna_spacing_h = lambda / 2;
      double antenna_spacing_v = lambda / 2;
      double k = 2 * M_PI / lambda;


      CartesianCoordinates* uePos = ue->GetMobilityModel ()->GetAbsolutePosition ();
      CartesianCoordinates* enbPos = enb->GetMobilityModel ()->GetWrapAroundPosition(uePos);
      double array_bearing = enb->GetPhy ()->GetAntennaParameters ()->GetBearing()*M_PI/180.0;

      // UE horizontal angle compared to antenna boresight
      double dst_angle_h = uePos->GetPolarAzimut(enbPos) - array_bearing;
      if ( dst_angle_h > M_PI )
        {
          dst_angle_h -= (2*M_PI);
        }



      double grid_angle_h_min = - M_PI / 3;
      double grid_angle_h_max = + M_PI / 3;

      if(std::getenv("USE_COVERSHIFT") != nullptr)
        {
          double sector_width = grid_angle_h_max - grid_angle_h_min;
          double threshold = 3./8.;

          EnbLtePhy* src = (EnbLtePhy*)(enb->GetPhy());
          int index;
          if (m_forceCoverShift != -1)
            {
              if (m_forceCoverShift<0 || m_forceCoverShift>=3)
                {
                  cout << "Error: forced invalid cover shift (" << m_forceCoverShift << ")!" << endl;
                }
              index = m_forceCoverShift;
            }
          else
            {
              index = FrameManager::Init()->GetCoverShiftIndex();
            }

          if(src->GetAntennaParameters()->GetBearing()==0)
            {
              if(index==0)
                {
                  grid_angle_h_min = grid_angle_h_min + sector_width * 0.32;
                  grid_angle_h_max = grid_angle_h_max - sector_width * 0.32;
                }
              else if(index==1)
                {
                  grid_angle_h_min = grid_angle_h_min + sector_width * 0.65;
                }
              else if(index==2)
                {
                  grid_angle_h_max = grid_angle_h_max - sector_width * 0.65;
                }
            }
          else if(src->GetAntennaParameters()->GetBearing()==120)
            {
              if(index==0)
                {
                  grid_angle_h_max = grid_angle_h_max - sector_width * 0.65;
                }
              else if(index==1)
                {
                  grid_angle_h_min = grid_angle_h_min + sector_width * 0.32;
                  grid_angle_h_max = grid_angle_h_max - sector_width * 0.32;
                }
              else if(index==2)
                {
                  grid_angle_h_min = grid_angle_h_min + sector_width * 0.65;
                }
            }
          else if(src->GetAntennaParameters()->GetBearing()==240)
            {
              if(index==0)
                {
                  grid_angle_h_min = grid_angle_h_min + sector_width * 0.65;
                }
              else if(index==1)
                {
                  grid_angle_h_max = grid_angle_h_max - sector_width * 0.65;
                }
              else if(index==2)
                {
                  grid_angle_h_min = grid_angle_h_min + sector_width * 0.32;
                  grid_angle_h_max = grid_angle_h_max - sector_width * 0.32;
                }
            }

        }

      // beam horizontal angle compared to antenna boresight
      // first beam is on the highest positive angle
      double grid_area_width = grid_angle_h_max - grid_angle_h_min;
      double beam_angle_h = grid_angle_h_min + grid_area_width / num_beams / 2 * (2 * beam_index + 1);
      double beam_to_dst_angle_h = beam_angle_h - dst_angle_h;
      double phase_shift_h = -k * antenna_spacing_h * sin( beam_angle_h );
      double psi_h = k * antenna_spacing_h * sin( dst_angle_h ) + phase_shift_h;
      double gain_h = abs( 1 / sqrt( beam_antennas_h ) * sin( beam_antennas_h * psi_h / 2 ) / sin( psi_h / 2 ) );

      double dst_angle_v = atan((enbPos->GetCoordinateZ() - uePos->GetCoordinateZ()) / (enbPos->GetDistance(uePos)));

      double beam_angle_v;
      if (beam_index%2 == 0)
        {
          beam_angle_v = 15*M_PI/180;
        }
      else
        {
          beam_angle_v = 25*M_PI/180;
        }

      double beam_to_dst_angle_v = beam_angle_v - dst_angle_v;
      double phase_shift_v = -k * antenna_spacing_v * sin( beam_angle_v );
      double psi_v = k * antenna_spacing_v * sin( dst_angle_v ) + phase_shift_v;
      double gain_v = abs( 1 / sqrt( beam_antennas_v ) * sin( beam_antennas_v * psi_v / 2 ) / sin( psi_v / 2 ) );

      gain = 20 * log10(gain_h * gain_v);


if(Simulator::Init()->Now()>0.01 &&false){
EnbLtePhy* src = (EnbLtePhy*)(enb->GetPhy());
cout
    << "beam " << beam_index
    << ", path " << path
    << ", BS bearing " << src->GetAntennaParameters()->GetBearing()
    << ", gain_h " << 20 * log10( gain_h ) << " dB"
    << ", dst_angle_h " << dst_angle_h*180/M_PI << "°"
    << ", beam_angle_h " << beam_angle_h*180/M_PI << "°"
    << ", beam_to_dst_angle_h " << beam_to_dst_angle_h*180/M_PI << "°"
    << ", gain " << gain << " dB"
    << ", eNB " << enb->GetIDNetworkNode()
    << ", UE " << ue->GetIDNetworkNode()
    << endl;

cout
    << "beam " << beam_index
    << ", path " << path
    << ", BS bearing " << src->GetAntennaParameters()->GetBearing()
    << ", gain_v " << 20 * log10( gain_v ) << " dB"
    << ", dst_angle_v " << dst_angle_v*180/M_PI << "°"
    << ", beam_angle_v " << beam_angle_v*180/M_PI << "°"
    << ", beam_to_dst_angle_v " << beam_to_dst_angle_v*180/M_PI << "°"
    << ", gain " << gain << " dB"
    << ", eNB " << enb->GetIDNetworkNode()
    << ", UE " << ue->GetIDNetworkNode()
    << endl;
}

    }
  else
    {
      gain = 0;
    }
  return gain;
}

vector< vector<double> >
ChannelRealization::GetLoss ()
{

DEBUG_LOG_START_1(LTE_SIM_TEST_PROPAGATION_LOSS_MODEL)
  cout << "\t  --> compute loss between "
          << GetSourceNode ()->GetIDNetworkNode () << " and "
          << GetDestinationNode ()->GetIDNetworkNode () << endl;
DEBUG_LOG_END

  if (NeedForUpdate()==true)
    {
      UpdateFastFading();
    }

  vector< vector<double> > loss;

  int now_ms = Simulator::Init()->Now () * 1000;
  int lastUpdate_ms = GetLastUpdate () * 1000;
  int index = now_ms - lastUpdate_ms;

  int nbOfPaths;
  if (hasFastFading()==true)
    {
      nbOfPaths = GetFastFading ()->size ();
    }
  else
    {
     nbOfPaths = 1;
    }
  int nbOfSubChannels = GetSourceNode ()->GetPhy ()->GetBandwidthManager ()->GetDlSubChannels ().size ();

  loss.resize (nbOfPaths);

  double pathloss = GetPathLoss ();
  double shadowing = GetShadowing ();
  double penetrationlLoss = GetPenetrationLoss ();
  double l;
  for (int i = 0; i < nbOfPaths; i++)
    {
      double beamformingGain = GetBeamformingGain (i);
      for (int j = 0; j < nbOfSubChannels; j++)
        {
          switch(m_channelModel)
            {
            case CHANNEL_MODEL_MACROCELL_URBAN:
            case CHANNEL_MODEL_3GPP_CASE1:
            case CHANNEL_MODEL_MACROCELL_SUB_URBAN:
            case CHANNEL_MODEL_MACROCELL_RURAL:
            case CHANNEL_MODEL_MICROCELL:
            case CHANNEL_MODEL_MACROCELL_URBAN_IMT:
            case CHANNEL_MODEL_MACROCELL_URBAN_IMT_3D:
            case CHANNEL_MODEL_MACROCELL_RURAL_IMT:
            case CHANNEL_MODEL_3GPP_DOWNLINK:
              l = beamformingGain - penetrationlLoss - shadowing - pathloss;
              if (hasFastFading()==true)
                {
                  l += (*GetFastFading ())[i][j][index];
                }
              break;
            case CHANNEL_MODEL_FEMTOCELL_URBAN:
            case CHANNEL_MODEL_WINNER_DOWNLINK:
              //ATTENZIONE double l = GetFastFading ()->at(i).at (j)[index] - GetPathLoss () - GetPenetrationLoss () - GetShadowing ();
              l = - GetPathLoss ();
              break;
            }

          loss.at (i).push_back (l);

DEBUG_LOG_START_2(LTE_SIM_TEST_PROPAGATION_LOSS_MODEL,LTE_SIM_TEST_GET_LOSS)
          cout << "\t\t mlp = " << (hasFastFading() ? GetFastFading ()->at(i).at (j)[index] : 0)
                    << " pl = " << GetPathLoss ()
                    << " pnl = " << GetPenetrationLoss()
                    << " sh = " << GetShadowing()
                    << " LOSS = " << l
                    << endl;
DEBUG_LOG_END
        }
    }

  return loss;
}

void
ChannelRealization::SetPenetrationLossMean(double plm) {
    m_penetrationLossMean = plm;
    m_penetrationLoss = GetGaussianRandomVariable(m_penetrationLossMean, m_penetrationLossStdDev);
}

double
ChannelRealization::GetPenetrationLossMean(void) {
    return m_penetrationLossMean;
}

void
ChannelRealization::SetPenetrationLossStdDev(double plsd) {
    m_penetrationLossStdDev = plsd;
    m_penetrationLoss = GetGaussianRandomVariable(m_penetrationLossMean, m_penetrationLossStdDev);
}

double
ChannelRealization::GetPenetrationLossStdDev(void) {
    return m_penetrationLossStdDev;
}

void
ChannelRealization::SetLoSState(bool state)
{
  m_isLosType = state;
}

bool
ChannelRealization::GetLoSState(void)
{
  return m_isLosType;
}

void
ChannelRealization::SetIndoorDistance(double d)
{
  m_indoorDistance = d;
}

double
ChannelRealization::GetIndoorDistance(void)
{
  return m_indoorDistance;
}

vector< vector<float> >
ChannelRealization::GetPhases ()
{
  vector< vector<float> > fastFadingPhases;
      int now_ms = Simulator::Init()->Now () * 1000;
      int lastUpdate_ms = GetLastUpdate () * 1000;
      int index = now_ms - lastUpdate_ms;

      fastFadingPhases.resize(m_fastFadingPhases->size());
      for (int i=0;i<(int)fastFadingPhases.size();i++)
        {
          fastFadingPhases.at(i).resize( m_fastFadingPhases->at(i).size() );
          for (int j=0;j<(int)m_fastFadingPhases->at(i).size();j++)
            {
              if ((*m_fastFadingPhases).at(i).at(j) != nullptr)
                {
                  fastFadingPhases[i][j] = (*m_fastFadingPhases)[i][j][index];
                }
              else
                {
                  fastFadingPhases[i][j] = 0;
                }
            }
        }

  return fastFadingPhases;
}

bool
ChannelRealization::NeedForUpdate (void)
{

DEBUG_LOG_START_2(LTE_SIM_TEST_PROPAGATION_LOSS_MODEL, LTE_SIM_TEST_PROPAGATION_LOSS_MODEL_NEED_UPDATE)
  cout << "Need for update ? Now " << Simulator::Init()->Now ()
          << " GetLastUpdate () " << GetLastUpdate ()
          << " GetSamplingPeriod () " << GetSamplingPeriod () << endl;
DEBUG_LOG_END

  if (Simulator::Init()->Now () >= (GetLastUpdate () + GetSamplingPeriod ()) - 0.001)
    {
      return true;
    }
  else
    {
      return false;
    }
}

ChannelRealization::FastFading*
ChannelRealization::GetFastFading ()
{
  return m_fastFading;
}

void
ChannelRealization::SetFastFading (ChannelRealization::FastFading* ff)
{
  m_fastFading = ff;
}

void
ChannelRealization::UpdateFastFading (void)
{
  if (hasFastFading()==false)
    {
      return;
    }

  //clear old realization
//  for(auto f : *m_fastFading)						// 并没有清空数组
//    {
//      f.clear();
//    }

  for(int i=0; i<(int)m_fastFading->size();i++)		// by zyb
    {
	  m_fastFading->at(i).resize(0);
    }

  for(int i=0; i<(int)m_fastFadingPhases->size();i++)
    {
      m_fastFadingPhases->at(i).resize(0);
    }

  int numbOfSubChannels = GetSourceNode ()->GetPhy ()->GetBandwidthManager ()->GetDlSubChannels ().size ();
  int samplingTime = GetSamplingPeriod () * 1000;
  int speed;
  UserEquipment* ue;

  if (GetSourceNode ()->GetNodeType () == NetworkNode::TYPE_UE)
    {
      ue = (UserEquipment*) GetSourceNode ();
      speed = ue->GetMobilityModel ()->GetSpeed ();
    }
  else if (GetDestinationNode ()->GetNodeType () == NetworkNode::TYPE_UE)
    {
      ue = (UserEquipment*) GetDestinationNode ();
      speed = ue->GetMobilityModel ()->GetSpeed ();
    }
  else
    {
      speed = 0;
    }

  int txMode = ue->GetTargetNodeRecord()->GetDlTxMode();

  // number of antennas for MIMO configurations
  int M;
  if (m_mbsfnRealization == true)
    {
      M = 1;
    }
  else
    {
      M = m_src->GetPhy()->GetTxAntennas();
    }
  int N = m_dst->GetPhy()->GetRxAntennas();


DEBUG_LOG_START_2(LTE_SIM_TEST_PROPAGATION_LOSS_MODEL,LTE_SIM_TEST_UPDATE_FAST_FADING)
  cout << "UpdateFastFading, "
          " speed " << speed <<
          " RBs " << numbOfSubChannels <<
          " samples " << samplingTime <<
          endl;
DEBUG_LOG_END

  float const * ff_gains;
  float const * ff_phases;

#if defined _channel_simple_
      for (int i = 0; i < numbOfSubChannels; i++)
        {
          for (int j = 0; j < M*N; j++)
            {
              int start_point_time = GetRandomVariable (19500);

              switch(speed)
                {
                case 3:
                  ff_gains = &(*ff_m2135_1x1_simple_3kmh_UMi[0])[0][start_point_time];
                  ff_phases = &(*ff_m2135_1x1_simple_3kmh_UMi_phase[0])[0][start_point_time];
                  break;
                case 30:
                  ff_gains = &(*ff_m2135_1x1_simple_30kmh_UMa[0])[0][start_point_time];
                  ff_phases = &(*ff_m2135_1x1_simple_30kmh_UMa_phase[0])[0][start_point_time];
                  break;
                case 120:
                  ff_gains = &(*ff_m2135_1x1_simple_120kmh_RMa[0])[0][start_point_time];
                  ff_phases = &(*ff_m2135_1x1_simple_120kmh_RMa_phase[0])[0][start_point_time];
                  break;
                default:
                  cout << "Error: missing speed value in ChannelRealization" << endl;
                  exit(1);
                  break;
                }
              m_fastFading->at(j).push_back (ff_gains);
              m_fastFadingPhases->at(j).push_back (ff_phases);
            }
        }
#elif defined _channel_advanced_
      int start_point_freq = 0;
      int start_point_time = GetRandomVariable (2500);
      int start_point_time_XPOL1 = GetRandomVariable (2500);
      int start_point_time_XPOL2 = GetRandomVariable (2500);
      int start_point_time_XPOL3 = GetRandomVariable (2500);

      for (int i = 0; i < numbOfSubChannels; i++)
        {
          if(M==1 && N==1)
            {
              switch(speed)
                {
                case 3:
                  ff_gains = &(*ff_m2135_1x1_antennatypeC_3kmh_UMi[0])[start_point_freq+i][start_point_time];
                  ff_phases = &(*ff_m2135_1x1_antennatypeC_3kmh_UMi_phase[0])[start_point_freq+i][start_point_time];
                  break;
                case 30:
                  ff_gains = &(*ff_m2135_1x1_antennatypeC_30kmh_UMa[0])[start_point_freq+i][start_point_time];
                  ff_phases = &(*ff_m2135_1x1_antennatypeC_30kmh_UMa_phase[0])[start_point_freq+i][start_point_time];
                  break;
                case 120:
                  ff_gains = &(*ff_m2135_1x1_antennatypeC_120kmh_RMa[0])[start_point_freq+i][start_point_time];
                  ff_phases = &(*ff_m2135_1x1_antennatypeC_120kmh_RMa_phase[0])[start_point_freq+i][start_point_time];
                  break;
                default:
                  cout << "Error: missing speed value in ChannelRealization: " << speed << endl;
                  exit(1);
                  break;
                }
              m_fastFading->at(0).push_back (ff_gains);
              m_fastFadingPhases->at(0).push_back (ff_phases);
            }
          else if(M==1 && N==2)
            {
              for (int j = 0; j < M*N; j++)
                {
                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_m2135_2x2_antennatypeC_3kmh_UMi[2*j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_2x2_antennatypeC_3kmh_UMi_phase[2*j])[start_point_freq+i][start_point_time];
                      break;
                    case 30:
                      ff_gains = &(*ff_m2135_2x2_antennatypeC_30kmh_UMa[2*j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_2x2_antennatypeC_30kmh_UMa_phase[2*j])[start_point_freq+i][start_point_time];
                      break;
                    case 120:
                      ff_gains = &(*ff_m2135_2x2_antennatypeC_120kmh_RMa[2*j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_2x2_antennatypeC_120kmh_RMa_phase[2*j])[start_point_freq+i][start_point_time];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 1x2" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==1 && N==4)
            {
              for (int j = 0; j < M*N; j++)
                {
                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_m2135_4x4_antennatypeC_3kmh_UMi[N*j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_4x4_antennatypeC_3kmh_UMi[N*j])[start_point_freq+i][start_point_time];
                      break;
                    case 30:
                      ff_gains = &(*ff_m2135_4x4_antennatypeC_30kmh_UMa[N*j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_4x4_antennatypeC_30kmh_UMa[N*j])[start_point_freq+i][start_point_time];
                      break;
                    case 120:
                      ff_gains = &(*ff_m2135_4x4_antennatypeC_120kmh_RMa[N*j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_4x4_antennatypeC_120kmh_RMa[N*j])[start_point_freq+i][start_point_time];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 1x4" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==1 && N==8)
            {
              for (int j = 0; j < M*N; j++)
                {
                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_m2135_8x8_antennatypeC_3kmh_UMi[N*j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_8x8_antennatypeC_3kmh_UMi[N*j])[start_point_freq+i][start_point_time];
                      break;
                    case 30:
                      ff_gains = &(*ff_m2135_8x8_antennatypeC_30kmh_UMa[N*j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_8x8_antennatypeC_30kmh_UMa[N*j])[start_point_freq+i][start_point_time];
                      break;
                    case 120:
                      ff_gains = &(*ff_m2135_8x8_antennatypeC_120kmh_RMa[N*j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_8x8_antennatypeC_120kmh_RMa[N*j])[start_point_freq+i][start_point_time];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 1x8" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==2 && N==1)
            {
              for (int j = 0; j < M*N; j++)
                {
                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_m2135_2x1_antennatypeC_3kmh_UMi[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_2x1_antennatypeC_3kmh_UMi_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    case 30:
                      ff_gains = &(*ff_m2135_2x1_antennatypeC_30kmh_UMa[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_2x1_antennatypeC_30kmh_UMa_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    case 120:
                      ff_gains = &(*ff_m2135_2x1_antennatypeC_120kmh_RMa[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_2x1_antennatypeC_120kmh_RMa_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 2x1" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==2 && N==2)
            {
              for (int j = 0; j < M*N; j++)
                {
                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_m2135_2x2_antennatypeC_3kmh_UMi[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_2x2_antennatypeC_3kmh_UMi_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    case 30:
                      ff_gains = &(*ff_m2135_2x2_antennatypeC_30kmh_UMa[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_2x2_antennatypeC_30kmh_UMa_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    case 120:
                      ff_gains = &(*ff_m2135_2x2_antennatypeC_120kmh_RMa[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_2x2_antennatypeC_120kmh_RMa_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 2x2" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==4 && N==2)
            {
              for (int j = 0; j < M*N; j++)
                {
                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_m2135_4x2_antennatypeC_3kmh_UMi[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_4x2_antennatypeC_3kmh_UMi_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    case 30:
                      ff_gains = &(*ff_m2135_4x2_antennatypeC_30kmh_UMa[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_4x2_antennatypeC_30kmh_UMa_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    case 120:
                      ff_gains = &(*ff_m2135_4x2_antennatypeC_120kmh_RMa[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_4x2_antennatypeC_120kmh_RMa_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 4x2" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==4 && N==4)
            {
              for (int j = 0; j < M*N; j++)
                {
                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_m2135_4x4_antennatypeC_3kmh_UMi[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_4x4_antennatypeC_3kmh_UMi_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    case 30:
                      ff_gains = &(*ff_m2135_4x4_antennatypeC_30kmh_UMa[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_4x4_antennatypeC_30kmh_UMa_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    case 120:
                      ff_gains = &(*ff_m2135_4x4_antennatypeC_120kmh_RMa[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_4x4_antennatypeC_120kmh_RMa_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 4x4" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==8 && N==1)
            {
              if (txMode == 11)
                {
                  for (int j = 0; j < M*N; j++)
                    {
                      switch(speed)
                        {
                        case 3:
                          ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_3kmh_UMa[j])[start_point_freq+i][start_point_time];
                          ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_3kmh_UMa_phase[j])[start_point_freq+i][start_point_time];
                          break;
                        case 30:
                          ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_30kmh_UMa[j])[start_point_freq+i][start_point_time];
                          ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_30kmh_UMa_phase[j])[start_point_freq+i][start_point_time];
                          break;
                        case 120:
                          ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_120kmh_UMa[j])[start_point_freq+i][start_point_time];
                          ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_120kmh_UMa_phase[j])[start_point_freq+i][start_point_time];
                          break;
                        default:
                          cout << "Error: missing speed value in ChannelRealization 8x1" << endl;
                          exit(1);
                          break;
                        }
                      m_fastFading->at(j).push_back (ff_gains);
                      m_fastFadingPhases->at(j).push_back (ff_phases);
                    }
                }
              else
                {
                  for (int j = 0; j < M*N; j++)
                    {
                      switch(speed)
                        {
                        case 3:
                          ff_gains = &(*ff_m2135_8x8_antennatypeC_3kmh_UMi[j])[start_point_freq+i][start_point_time];
                          ff_phases = &(*ff_m2135_8x8_antennatypeC_3kmh_UMi_phase[j])[start_point_freq+i][start_point_time];
                          break;
                        case 30:
                          ff_gains = &(*ff_m2135_8x8_antennatypeC_30kmh_UMa[j])[start_point_freq+i][start_point_time];
                          ff_phases = &(*ff_m2135_8x8_antennatypeC_30kmh_UMa_phase[j])[start_point_freq+i][start_point_time];
                          break;
                        case 120:
                          ff_gains = &(*ff_m2135_8x8_antennatypeC_120kmh_RMa[j])[start_point_freq+i][start_point_time];
                          ff_phases = &(*ff_m2135_8x8_antennatypeC_120kmh_RMa_phase[j])[start_point_freq+i][start_point_time];
                          break;
                        default:
                          cout << "Error: missing speed value in ChannelRealization 8x1" << endl;
                          exit(1);
                          break;
                        }
                      m_fastFading->at(j).push_back (ff_gains);
                      m_fastFadingPhases->at(j).push_back (ff_phases);
                    }
                }
            }
          else if(M==8 && N==2)
            {
              if (txMode == 11)
                {
                  for (int j = 0; j < M*N; j++)
                    {
                      int start_point_time_XPOL;
                      switch (j/(M*N/4))
                        {
                          case 0: start_point_time_XPOL = start_point_time; break;
                          case 1: start_point_time_XPOL = start_point_time_XPOL1; break;
                          case 2: start_point_time_XPOL = start_point_time_XPOL2; break;
                          case 3: start_point_time_XPOL = start_point_time_XPOL3; break;
                          default:
                            cout << "Error: unhandled case in ChannelRealization" << endl;
                            exit(1);
                            break;
                        }
                      switch(speed)
                        {
                        case 3:
                          ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_3kmh_UMa[j])[start_point_freq+i][start_point_time_XPOL];
                          ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_3kmh_UMa_phase[j])[start_point_freq+i][start_point_time_XPOL];
                          break;
                        case 30:
                          ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_30kmh_UMa[j])[start_point_freq+i][start_point_time_XPOL];
                          ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_30kmh_UMa_phase[j])[start_point_freq+i][start_point_time_XPOL];
                          break;
                        case 120:
                          ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_120kmh_UMa[j])[start_point_freq+i][start_point_time_XPOL];
                          ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_120kmh_UMa_phase[j])[start_point_freq+i][start_point_time_XPOL];
                          break;
                        default:
                          cout << "Error: missing speed value in ChannelRealization 8x2" << endl;
                          exit(1);
                          break;
                        }
                      m_fastFading->at(j).push_back (ff_gains);
                      m_fastFadingPhases->at(j).push_back (ff_phases);
                    }
                }
              else
                {
                  for (int j = 0; j < M*N; j++)
                    {
                      switch(speed)
                        {
                        case 3:
                          ff_gains = &(*ff_m2135_8x8_antennatypeC_3kmh_UMi[j])[start_point_freq+i][start_point_time];
                          ff_phases = &(*ff_m2135_8x8_antennatypeC_3kmh_UMi_phase[j])[start_point_freq+i][start_point_time];
                          break;
                        case 30:
                          ff_gains = &(*ff_m2135_8x8_antennatypeC_30kmh_UMa[j])[start_point_freq+i][start_point_time];
                          ff_phases = &(*ff_m2135_8x8_antennatypeC_30kmh_UMa_phase[j])[start_point_freq+i][start_point_time];
                          break;
                        case 120:
                          ff_gains = &(*ff_m2135_8x8_antennatypeC_120kmh_RMa[j])[start_point_freq+i][start_point_time];
                          ff_phases = &(*ff_m2135_8x8_antennatypeC_120kmh_RMa_phase[j])[start_point_freq+i][start_point_time];
                          break;
                        default:
                          cout << "Error: missing speed value in ChannelRealization 8x2" << endl;
                          exit(1);
                          break;
                        }
                      m_fastFading->at(j).push_back (ff_gains);
                      m_fastFadingPhases->at(j).push_back (ff_phases);
                    }
                }
            }


          else if(M==16 && N==1)
            {
              for (int j = 0; j < M*N; j++)
                {

                  int start_point_time_XPOL;
                  int j2;
                  switch (j/(M*N/2))
                    {
                      case 0:
                        start_point_time_XPOL = start_point_time;
                        j2 = j;
                        break;
                      case 1:
                        start_point_time_XPOL = start_point_time_XPOL1;
                        j2 = j%(M/2);
                        break;
                      default:
                        cout << "Error: unhandled case in ChannelRealization" << endl;
                        exit(1);
                        break;
                    }

                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_3kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_3kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 30:
                      ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_30kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_30kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 120:
                      ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_120kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_120kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 16x1" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==16 && N==2)
            {
              if (txMode == 11)
                {
                  for (int j = 0; j < M*N; j++)
                    {
                      int start_point_time_XPOL;
                      int j2;
                      switch (j/(M*N/4))
                        {
                          case 0:
                            start_point_time_XPOL = start_point_time;
                            j2 = j;
                            break;
                          case 1:
                            start_point_time_XPOL = start_point_time_XPOL1;
                            j2 = j - (M*N/4);
                            break;
                          case 2:
                            start_point_time_XPOL = start_point_time;
                            j2 = j - (M*N/4);
                            break;
                          case 3:
                            start_point_time_XPOL = start_point_time_XPOL1;
                            j2 = j - (M*N/2);
                            break;
                          default:
                            cout << "Error: unhandled case in ChannelRealization" <<endl;
                            exit(1);
                            break;
                        }
                      switch(speed)
                        {
                        case 3:
                          ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_3kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                          ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_3kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                          break;
                        case 30:
                          ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_30kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                          ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_30kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                          break;
                        case 120:
                          ff_gains = &(*ff_3gpp3d_8x2_antennatypeC_GoB_120kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                          ff_phases = &(*ff_3gpp3d_8x2_antennatypeC_GoB_120kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                          break;
                        default:
                          cout << "Error: missing speed value in ChannelRealization 16x2" << endl;
                          exit(1);
                          break;
                        }

                      m_fastFading->at(j).push_back (ff_gains);
                      m_fastFadingPhases->at(j).push_back (ff_phases);
                    }
                }
              else
                {
                  for (int j = 0; j < M*N; j++)
                    {
                      int start_point_time_XPOL;
                      switch (j/(M*N/4))
                        {
                          case 0: start_point_time_XPOL = start_point_time; break;
                          case 1: start_point_time_XPOL = start_point_time_XPOL1; break;
                          case 2: start_point_time_XPOL = start_point_time_XPOL2; break;
                          case 3: start_point_time_XPOL = start_point_time_XPOL3; break;
                          default:
                            cout << "Error: unhandled case in ChannelRealization" << endl;
                            exit(1);
                            break;
                        }
                      switch(speed)
                        {
                        case 3:
                          ff_gains = &(*ff_m2135_8x8_antennatypeC_3kmh_UMi[j])[start_point_freq+i][start_point_time_XPOL];
                          ff_phases = &(*ff_m2135_8x8_antennatypeC_3kmh_UMi_phase[j])[start_point_freq+i][start_point_time_XPOL];
                          break;
                        case 30:
                          ff_gains = &(*ff_m2135_8x8_antennatypeC_30kmh_UMa[j])[start_point_freq+i][start_point_time_XPOL];
                          ff_phases = &(*ff_m2135_8x8_antennatypeC_30kmh_UMa_phase[j])[start_point_freq+i][start_point_time_XPOL];
                          break;
                        case 120:
                          ff_gains = &(*ff_m2135_8x8_antennatypeC_120kmh_RMa[j])[start_point_freq+i][start_point_time_XPOL];
                          ff_phases = &(*ff_m2135_8x8_antennatypeC_120kmh_RMa_phase[j])[start_point_freq+i][start_point_time_XPOL];
                          break;
                        default:
                          cout << "Error: missing speed value in ChannelRealization 16x2" << endl;
                          exit(1);
                          break;
                        }

                      m_fastFading->at(j).push_back (ff_gains);
                      m_fastFadingPhases->at(j).push_back (ff_phases);
                    }
                }
            }
          else if(M==32 && N==1)
            {
              for (int j = 0; j < M*N; j++)
                {
                  int start_point_time_XPOL;
                      int j2;
                      switch (j/(M*N/2))
                        {
                          case 0:
                            start_point_time_XPOL = start_point_time;
                            j2 = j;
                            break;
                          case 1:
                            start_point_time_XPOL = start_point_time_XPOL1;
                            j2 = j%(M/2);
                            break;
                          default:
                            cout << "Error: unhandled case in ChannelRealization" << endl;
                            exit(1);
                            break;
                        }

                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_3kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_3kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 30:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_30kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_30kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 120:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_120kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_120kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 250:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_250kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_250kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 500:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_500kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_500kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 32x1" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==32 && N==2)
            {
              for (int j = 0; j < M*N; j++)
                {
                  int start_point_time_XPOL;
                  int j2;
                  switch (j/(M*N/4))
                    {
                      case 0:
                        start_point_time_XPOL = start_point_time;
                        j2 = j;
                        break;
                      case 1:
                        start_point_time_XPOL = start_point_time_XPOL1;
                        j2 = j - (M*N/4);
                        break;
                      case 2:
                        start_point_time_XPOL = start_point_time;
                        j2 = j - (M*N/4);
                        break;
                      case 3:
                        start_point_time_XPOL = start_point_time_XPOL1;
                        j2 = j - (M*N/2);
                        break;
                      default:
                        cout << "Error: unhandled case in ChannelRealization" <<endl;
                        exit(1);
                        break;
                    }
                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_3kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_3kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 30:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_30kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_30kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 120:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_120kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_120kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 250:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_250kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_250kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 500:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_500kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_500kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 32x2" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==32 && N==4)
            {
              for (int j = 0; j < M*N; j++)
                {
                  int start_point_time_XPOL;
                  int j2 = j % (M*N/4);
                  switch (j/(M*N/4))
                    {
                      case 0:
                        start_point_time_XPOL = start_point_time;
                        break;
                      case 1:
                        start_point_time_XPOL = start_point_time_XPOL1;
                        break;
                      case 2:
                        start_point_time_XPOL = start_point_time_XPOL2;
                        break;
                      case 3:
                        start_point_time_XPOL = start_point_time_XPOL3;
                        break;
                      default:
                        cout << "Error: unhandled case in ChannelRealization" <<endl;
                        exit(1);
                        break;
                    }
                  switch(speed)
                    {
                      break;
                    case 3:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_3kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_3kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 30:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_30kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_30kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 120:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_120kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_120kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 250:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_250kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_250kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 500:
                      ff_gains = &(*ff_3gpp3d_16x2_antennatypeC_GoB_500kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x2_antennatypeC_GoB_500kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 32x4" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==32 && N==8)
            {
              for (int j = 0; j < M*N; j++)
                {
                  int start_point_time_XPOL;
                  int j2 = j % (M*N/4);
                  switch (j/(M*N/4))
                    {
                      case 0:
                        start_point_time_XPOL = start_point_time;
                        break;
                      case 1:
                        start_point_time_XPOL = start_point_time_XPOL1;
                        break;
                      case 2:
                        start_point_time_XPOL = start_point_time_XPOL2;
                        break;
                      case 3:
                        start_point_time_XPOL = start_point_time_XPOL3;
                        break;
                      default:
                        cout << "Error: unhandled case in ChannelRealization" <<endl;
                        exit(1);
                        break;
                    }
                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_3gpp3d_16x4_antennatypeC_GoB_3kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x4_antennatypeC_GoB_3kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 30:
                      ff_gains = &(*ff_3gpp3d_16x4_antennatypeC_GoB_30kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x4_antennatypeC_GoB_30kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    case 120:
                      ff_gains = &(*ff_3gpp3d_16x4_antennatypeC_GoB_120kmh_UMa[j2])[start_point_freq+i][start_point_time_XPOL];
                      ff_phases = &(*ff_3gpp3d_16x4_antennatypeC_GoB_120kmh_UMa_phase[j2])[start_point_freq+i][start_point_time_XPOL];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 32x8" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else if(M==8 && N==8)
            {
              for (int j = 0; j < M*N; j++)
                {
                  switch(speed)
                    {
                    case 3:
                      ff_gains = &(*ff_m2135_8x8_antennatypeC_3kmh_UMi[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_8x8_antennatypeC_3kmh_UMi_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    case 30:
                      ff_gains = &(*ff_m2135_8x8_antennatypeC_30kmh_UMa[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_8x8_antennatypeC_30kmh_UMa_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    case 120:
                      ff_gains = &(*ff_m2135_8x8_antennatypeC_120kmh_RMa[j])[start_point_freq+i][start_point_time];
                      ff_phases = &(*ff_m2135_8x8_antennatypeC_120kmh_RMa_phase[j])[start_point_freq+i][start_point_time];
                      break;
                    default:
                      cout << "Error: missing speed value in ChannelRealization 8x8" << endl;
                      exit(1);
                      break;
                    }
                  m_fastFading->at(j).push_back (ff_gains);
                  m_fastFadingPhases->at(j).push_back (ff_phases);
                }
            }
          else
            {
              cout << " ERROR: M2135 Model, incorrect MIMO configuration " << M << "x" << N << endl;
              exit (1);
            }
        }
#else
      cout << "Error: invalid combination of _channel_simple_ and _channel_advanced_" << endl;
#endif
  SetLastUpdate ();
}

void
ChannelRealization::SetChannelModel(ChannelModel model)
{
  m_channelModel = model;
}

ChannelRealization::ChannelModel
ChannelRealization::GetChannelModel(void)
{
  return m_channelModel;
}

bool
ChannelRealization::isMbsfnRealization(void)
{
  return m_mbsfnRealization;
}
