# 添加损耗 #

[ReceivedSignal* PropagationLossModel::AddLossModel (NetworkNode* src, NetworkNode* dst, TransmittedSignal* txSignal)](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/channel/propagation-model/propagation-loss-model.cpp#L81)

[接收功率计算](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/channel/propagation-model/propagation-loss-model.cpp#L184)

The loss propagation model for LTE networks is based on a on a combination of four different models:
- the path loss, 传播损耗
- the penetration loss, 穿透损耗
- the shadowind, 阴影损耗
- the multipath, 多径损耗/快衰落

The rxPsd will be obtained considering, for each sub channel, the following relations:
rxPsd (i) = txPsd (i) + m(i,t) - sh(i,t) - pnl(i,t) - pl (a,b);
rxPsd (i) = txPsd(i) + multipath(i,t) - shadowing(i,t) - penetrationlLoss(i,t) - pathloss(a, b)
                                l = beamformingGain - penetrationlLoss - shadowing - pathloss;
where i is the i-th sub-channel and t is the current time (Simulator::Now()).


vector< vector<double> >
ChannelRealization::GetLoss ()
是否升级快衰落
更新快衰落矩阵


# 四种损耗 #

损耗计算

[vector< vector<double> > ChannelRealization::GetLoss ()](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/channel/propagation-model/channel-realization.cpp#L1003)


![](https://raw.githubusercontent.com/wu0hgl/nb-iot/master/report/picture/channel_realization_3.png)



# multipath #

更新快衰落数组的值
[void ChannelRealization::UpdateFastFading (void)](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/channel/propagation-model/channel-realization.cpp#L1193) 


错误修改
![](https://raw.githubusercontent.com/wu0hgl/nb-iot/master/report/picture/channel_realization_1.png)

快衰落值的获取
![](https://raw.githubusercontent.com/wu0hgl/nb-iot/master/report/picture/channel_realization_2.png)


[快衰落矩阵](https://github.com/wu0hgl/nb-iot/tree/master/src/channel/propagation-model/FastFadingRealization/m2135_model)



# pathloss #

[double ChannelRealization::GetPathLoss (void)](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/channel/propagation-model/channel-realization.cpp#L450)

```C++
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
```

# shadowing #

[double ChannelRealization::GetShadowing (void)](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/channel/propagation-model/channel-realization.cpp#L750)


[void ChannelRealization::ShortTermUpdate(void)](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/channel/propagation-model/channel-realization.cpp#L327)

高斯随机变量获取方法: 
[static double GetGaussianRandomVariable(double mean=0, double stdDev=1)](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/utility/GaussianRandomVariable.h)

# penetrationlLoss #

[double ChannelRealization::GetPenetrationLoss (void)](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/channel/propagation-model/channel-realization.cpp#L775)


# 有效sinr映射 #

具体映射方法有些疑问

[static double GetMiesmEffectiveSinr (const vector <double> &sinrs)](https://github.com/wu0hgl/nb-iot/blob/96f04edbcc771c17647038c37e1f4969c69d4a7f/src/utility/miesm-effective-sinr.h#L128)


# 输出值 #
```shell
UpdateFastFading,  speed 3 RBs 49 samples 500
		 mlp = 2.1611 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -123.541
		 mlp = -3.2615 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -128.964
		 mlp = 1.3459 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -124.357
		 mlp = 5.0087 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -120.694
		 mlp = -12.106 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -137.809
		 mlp = -0.5541 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -126.257
		 mlp = 0.32092 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -125.382
		 mlp = -0.69102 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -126.394
		 mlp = -4.7123 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -130.415
		 mlp = 0.95332 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -124.749
		 mlp = 1.0476 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -124.655
		 mlp = 4.459 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -121.244
		 mlp = 3.6157 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -122.087
		 mlp = 6.3129 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -119.39
		 mlp = 2.3462 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -123.356
		 mlp = 2.2474 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -123.455
		 mlp = 2.803 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -122.9
		 mlp = 9.1989 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -116.504
		 mlp = 5.2741 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -120.428
		 mlp = 6.7458 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -118.957
		 mlp = 4.2593 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -121.443
		 mlp = 1.8303 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -123.872
		 mlp = 2.1041 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -123.598
		 mlp = 4.0167 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -121.686
		 mlp = 3.5738 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -122.129
		 mlp = -0.44429 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -126.147
		 mlp = 0.7487 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -124.954
		 mlp = -2.5939 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -128.296
		 mlp = -2.0707 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -127.773
		 mlp = -7.6814 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -133.384
		 mlp = 1.01 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -124.693
		 mlp = 0.96869 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -124.734
		 mlp = 3.2267 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -122.476
		 mlp = -8.3883 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -134.091
		 mlp = 1.6326 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -124.07
		 mlp = 3.4695 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -122.233
		 mlp = 1.1412 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -124.561
		 mlp = 5.3065 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -120.396
		 mlp = 2.2624 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -123.44
		 mlp = -2.6511 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -128.354
		 mlp = -0.57634 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -126.279
		 mlp = -4.4131 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -130.116
		 mlp = -2.9475 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -128.65
		 mlp = 2.6382 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -123.064
		 mlp = 0.52939 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -125.173
		 mlp = -4.1177 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -129.82
		 mlp = 3.4253 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -122.277
		 mlp = 4.6982 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -121.004
		 mlp = -0.21909 pl = 113.143 pnl = 10 sh = 2.56 LOSS = -125.922
tx sub channels 49 loss sub channels 49
		 path 0 sub channel = 0 rxSignalValues = -7 loss = -123.541 rxPower = -130.541
		 path 0 sub channel = 1 rxSignalValues = 0 loss = -128.964 rxPower = -128.964
		 path 0 sub channel = 2 rxSignalValues = 0 loss = -124.357 rxPower = -124.357
		 path 0 sub channel = 3 rxSignalValues = 0 loss = -120.694 rxPower = -120.694
		 path 0 sub channel = 4 rxSignalValues = 0 loss = -137.809 rxPower = -137.809
		 path 0 sub channel = 5 rxSignalValues = 0 loss = -126.257 rxPower = -126.257
		 path 0 sub channel = 6 rxSignalValues = 0 loss = -125.382 rxPower = -125.382
		 path 0 sub channel = 7 rxSignalValues = 0 loss = -126.394 rxPower = -126.394
		 path 0 sub channel = 8 rxSignalValues = 0 loss = -130.415 rxPower = -130.415
		 path 0 sub channel = 9 rxSignalValues = 0 loss = -124.749 rxPower = -124.749
		 path 0 sub channel = 10 rxSignalValues = 0 loss = -124.655 rxPower = -124.655
		 path 0 sub channel = 11 rxSignalValues = 0 loss = -121.244 rxPower = -121.244
		 path 0 sub channel = 12 rxSignalValues = 0 loss = -122.087 rxPower = -122.087
		 path 0 sub channel = 13 rxSignalValues = 0 loss = -119.39 rxPower = -119.39
		 path 0 sub channel = 14 rxSignalValues = 0 loss = -123.356 rxPower = -123.356
		 path 0 sub channel = 15 rxSignalValues = 0 loss = -123.455 rxPower = -123.455
		 path 0 sub channel = 16 rxSignalValues = 0 loss = -122.9 rxPower = -122.9
		 path 0 sub channel = 17 rxSignalValues = 0 loss = -116.504 rxPower = -116.504
		 path 0 sub channel = 18 rxSignalValues = 0 loss = -120.428 rxPower = -120.428
		 path 0 sub channel = 19 rxSignalValues = 0 loss = -118.957 rxPower = -118.957
		 path 0 sub channel = 20 rxSignalValues = 0 loss = -121.443 rxPower = -121.443
		 path 0 sub channel = 21 rxSignalValues = 0 loss = -123.872 rxPower = -123.872
		 path 0 sub channel = 22 rxSignalValues = 0 loss = -123.598 rxPower = -123.598
		 path 0 sub channel = 23 rxSignalValues = 0 loss = -121.686 rxPower = -121.686
		 path 0 sub channel = 24 rxSignalValues = 0 loss = -122.129 rxPower = -122.129
		 path 0 sub channel = 25 rxSignalValues = 0 loss = -126.147 rxPower = -126.147
		 path 0 sub channel = 26 rxSignalValues = 0 loss = -124.954 rxPower = -124.954
		 path 0 sub channel = 27 rxSignalValues = 0 loss = -128.296 rxPower = -128.296
		 path 0 sub channel = 28 rxSignalValues = 0 loss = -127.773 rxPower = -127.773
		 path 0 sub channel = 29 rxSignalValues = 0 loss = -133.384 rxPower = -133.384
		 path 0 sub channel = 30 rxSignalValues = 0 loss = -124.693 rxPower = -124.693
		 path 0 sub channel = 31 rxSignalValues = 0 loss = -124.734 rxPower = -124.734
		 path 0 sub channel = 32 rxSignalValues = 0 loss = -122.476 rxPower = -122.476
		 path 0 sub channel = 33 rxSignalValues = 0 loss = -134.091 rxPower = -134.091
		 path 0 sub channel = 34 rxSignalValues = 0 loss = -124.07 rxPower = -124.07
		 path 0 sub channel = 35 rxSignalValues = 0 loss = -122.233 rxPower = -122.233
		 path 0 sub channel = 36 rxSignalValues = 0 loss = -124.561 rxPower = -124.561
		 path 0 sub channel = 37 rxSignalValues = 0 loss = -120.396 rxPower = -120.396
		 path 0 sub channel = 38 rxSignalValues = 0 loss = -123.44 rxPower = -123.44
		 path 0 sub channel = 39 rxSignalValues = 0 loss = -128.354 rxPower = -128.354
		 path 0 sub channel = 40 rxSignalValues = 0 loss = -126.279 rxPower = -126.279
		 path 0 sub channel = 41 rxSignalValues = 0 loss = -130.116 rxPower = -130.116
		 path 0 sub channel = 42 rxSignalValues = 0 loss = -128.65 rxPower = -128.65
		 path 0 sub channel = 43 rxSignalValues = 0 loss = -123.064 rxPower = -123.064
		 path 0 sub channel = 44 rxSignalValues = 0 loss = -125.173 rxPower = -125.173
		 path 0 sub channel = 45 rxSignalValues = 0 loss = -129.82 rxPower = -129.82
		 path 0 sub channel = 46 rxSignalValues = 0 loss = -122.277 rxPower = -122.277
		 path 0 sub channel = 47 rxSignalValues = 0 loss = -121.004 rxPower = -121.004
		 path 0 sub channel = 48 rxSignalValues = 0 loss = -125.922 rxPower = -125.922
```


# 修改函数 #

[channel-realization.cpp](https://github.com/wu0hgl/nb-iot/blob/master/src/channel/propagation-model/channel-realization.cpp)