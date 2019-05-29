# single-cell-with-interference.h #

这里可设置四种应用层
```C++
VoIP VoIPApplication[nbVoIP*nbCell*nbUE];
TraceBased VideoApplication[nbVideo*nbCell*nbUE];
InfiniteBuffer BEApplication[nbBE*nbCell*nbUE];
CBR CBRApplication[nbCBR*nbCell*nbUE];

// create application
VoIPApplication[voipApplication].SetSource (gw);
VoIPApplication[voipApplication].SetDestination (ue);
VoIPApplication[voipApplication].SetApplicationID (applicationID);

// create application
VideoApplication[videoApplication].SetSource (gw);
VideoApplication[videoApplication].SetDestination (ue);
VideoApplication[videoApplication].SetApplicationID (applicationID);

// create application
BEApplication[beApplication].SetSource (gw);
BEApplication[beApplication].SetDestination (ue);
BEApplication[beApplication].SetApplicationID (applicationID);

// create application
CBRApplication[cbrApplication].SetSource (gw);
CBRApplication[cbrApplication].SetDestination (ue);
CBRApplication[cbrApplication].SetApplicationID (applicationID);
```

每个应用层都有QoSParameters, 可设置发送间隔, 在nb-cell.h中发送间隔是在刚好是发送和接收的时长
```C++
QoSParameters *qosParameters = new QoSParameters ();	// QoSParameters在初始化时默认延迟是负无穷
qosParameters->SetMaxDelay (flow_duration);				// 不是没有用到qos, 只是把它给延迟到发送结束时
CBRApplication[cbrApplication].SetQoSParameters (qosParameters);
```

```shell
#  								nbCells  radius  nbUE  nbVoIP  nbVideo  nbBE  nbCBR  sched_type  frame_struct  speed  maxDelay  videoBitRate  seed
./5G-simulator SingleCellWithI    19       0.5    10      1        1      1      0        1           1          3      0.04        128

./5G-simulator SingleCellWithI    1        0.5    1       0        0      0      1        1           1          3      0.04        128

此时的输出
TX CBR ID 251 B 0 SIZE 10 SRC -1 DST 1 T 11.097 0
**** NO PHY ERROR (node 1) ****
PHY_RX SRC 0 DST 1 X -445.784 Y 16.1416 SINR 16.9139 RB 1 MCS 15 SIZE 256 ERR 0 T 11.098
RX CBR ID 251 B 0 SIZE 5 SRC -1 DST 1 D 0.001 0
```

修改发送源是ue, 接收源是gw(nb-cell.h的接收源是enb), 此时只有TX没有RX
```shell
CBRApplication[cbrApplication].SetSource (ue);
CBRApplication[cbrApplication].SetDestination (gw);

#  								nbCells  radius  nbUE  nbVoIP  nbVideo  nbBE  nbCBR  sched_type  frame_struct  speed  maxDelay  videoBitRate  seed
./5G-simulator SingleCellWithI    1        0.5    1       0        0      0      1        1           1          3      0.04         128
```

# nb-cell.h #

nb-cell.h中包的发送的间隔`CBRApplication[cbrApplication].SetInterval ((double) CBR_interval);`设置小了, 就ue和enb不会断开, 只有一个接入过程, 之后一直都在tx和rx之中

原文件中的源和目的的设置分别为enb和ue
```C++
CBRApplication[cbrApplication].SetSource (enb);
CBRApplication[cbrApplication].SetDestination (ue);
```

下行的测试, 发送源分别设置为gw或enb输出结果, 这时报错下行信道没有实现
```C++
CBRApplication[cbrApplication].SetSource (gw);
CBRApplication[cbrApplication].SetDestination (ue);

zyb@server:~/nb_iot$ ./5G-simulator nbCell    1      300     1    1      5         1        3.75      1        5          128      1
Simulation with SEED = 1
Duration: 300 flow: 300
TTI Length: 32ms 
Scheduler NB RR 
Created eNB - id 1 position (0;0)
ZONE 6 LOW EDGE 363 DISTANCE 398.853
Created UE - id 2 position 282.032 282.032
CREATED CBR APPLICATION, ID 0
ERROR: Channel Realization Not Found!
```

```shell
CBRApplication[cbrApplication].SetSource (enb);
CBRApplication[cbrApplication].SetDestination (ue);

zyb@server:~/nb_iot$ ./5G-simulator nbCell    1      300     1    1      5         1        3.75      1        50          128      1
Simulation with SEED = 1
Duration: 300 flow: 300
TTI Length: 32ms 
Scheduler NB RR 
Created eNB - id 1 position (0;0)
ZONE 6 LOW EDGE 363 DISTANCE 398.853
Created UE - id 2 position 282.032 282.032
CREATED CBR APPLICATION, ID 0
ERROR: Channel Realization Not Found!
```

参考test-demo.h的信道实例化设置
```C++
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

ChannelRealization *prev_c_dl, *prev_c_ul;
for (auto enb : *networkManager->GetENodeBContainer ())
{
  ChannelRealization* c_dl = new ChannelRealization (enb, ue, model);		// 信道模型, 这里通过
  enb->GetPhy ()->GetDlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_dl);	// 这里设置下行信道
  ChannelRealization* c_ul = new ChannelRealization (ue, enb, model);
  enb->GetPhy ()->GetUlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_ul);	// 这里设置上行信道

  c_dl->SetShadowingStddev(shadowingStddev);
  c_ul->SetShadowingStddev(shadowingStddev);

  if(ue->IsIndoor())
    {
      c_dl->SetPenetrationLossMean(penetrationLossMeanIndoor);
      c_ul->SetPenetrationLossMean(penetrationLossMeanIndoor);
    }
  else
    {
      c_dl->SetPenetrationLossMean(penetrationLossMeanOutdoor);
      c_ul->SetPenetrationLossMean(penetrationLossMeanOutdoor);
    }
  c_dl->SetPenetrationLossStdDev(penetrationLossStdDev);
  c_ul->SetPenetrationLossStdDev(penetrationLossStdDev);

  // syncronize LoS state and shadowing for eNBs at the same site
  if (prevEnb != nullptr && enb->GetCell()->GetIdCell() == prevEnb->GetCell()->GetIdCell())
    {
      c_dl->SetLoSState(prev_c_dl->GetLoSState());
      c_ul->SetLoSState(prev_c_ul->GetLoSState());
      c_dl->SetShadowing(prev_c_dl->GetShadowing());
      c_ul->SetShadowing(prev_c_ul->GetShadowing());
    }
  prevEnb = enb;
  prev_c_dl = c_dl;
  prev_c_ul = c_ul;
}

// ChannelRealization简单化设置
ChannelRealization* c_dl = new ChannelRealization (enb, ue, model);
enb->GetPhy ()->GetDlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_dl);
ChannelRealization* c_ul = new ChannelRealization (ue, enb, model);
enb->GetPhy ()->GetUlChannel ()->GetPropagationLossModel ()->AddChannelRealization (c_ul);
```

这里加入信道的实例化, 这时ue的速度不能设为0, 只能在3, 30, 120三者之中选一个, 其他值报错
```C++
UserEquipment* ue = new UserEquipment (idUE, posX, posY, 
										3,	// speed 
										speedDirection, cell,	enb,	0, //handover false!
										Mobility::CONSTANT_POSITION);
```

还要加入网关设置
```C++
//Create GW
Gateway *gw = new Gateway ();
networkManager->GetGatewayContainer ()->push_back (gw);
```

应用层的发送源和接收源
```C++
ftpApplication[ftpApplicationID].SetSource (gw);
ftpApplication[ftpApplicationID].SetDestination (ue);
```

加了InfiniteBuffer应用层后没有随机接入过程, CBR只有TX和RX, 没有RACH的输出, 好像没接入的过程了, 这个是修改nb-cell.h之后的输出, 观察SRC和DST

```shell
Scheduler NB RR
Created eNB - id 1 position (0;0)
ZONE 6 LOW EDGE 363 DISTANCE 398.853
Created UE - id 2 position 282.032 282.032
CREATED BE APPLICATION, ID 0
CREATED CBR APPLICATION, ID 1
TX INF_BUF ID 0 B 1 SIZE 1490 SRC 1 DST 2 T 0.012 0
TX INF_BUF ID 1 B 1 SIZE 1490 SRC 1 DST 2 T 0.012 0
TX INF_BUF ID 2 B 1 SIZE 1490 SRC 1 DST 2 T 0.012 0
TX INF_BUF ID 3 B 1 SIZE 65 SRC 1 DST 2 T 0.012 0
**** NO PHY ERROR (node 2) ****
PHY_RX SRC 1 DST 2 X 282.032 Y 282.032 SINR 29.214 RB 49 MCS 28 SIZE 36696 ERR 0 T 0.013
RX INF_BUF ID 0 B 1 SIZE 1490 SRC -1 DST 2 D 0.001 0
RX INF_BUF ID 1 B 1 SIZE 1490 SRC -1 DST 2 D 0.001 0
RX INF_BUF ID 2 B 1 SIZE 1490 SRC -1 DST 2 D 0.001 0
RX INF_BUF ID 3 B 1 SIZE 65 SRC -1 DST 2 D 0.001 0
```

```shell
Created Cell, id 0, position: 0 0 radius 0.144338
Created UE - id 57 position 25 0
...
...
TX INF_BUF ID 0 B 0 SIZE 704 SRC 0 DST 57 T 0.067 0
**** NO PHY ERROR (node 57) ****
PHY_RX SRC 0 DST 57 X 25.5583 Y 0 SINR -1.33607 RB 100 MCS 3 SIZE 5736 ERR 0 T 0.068
RX INF_BUF ID 0 B 0 SIZE 704 SRC -1 DST 57 D 0.001 0
```

下一步想法是在FullbandCqiManager设置断点, 查看都调用哪些参数, 然后观察这些参数是否变化, 能否写死等
```C++
FullbandCqiManager *cqiManager = new FullbandCqiManager ();
cqiManager->SetCqiReportingMode (CqiManager::PERIODIC);
cqiManager->SetReportingInterval (1);
cqiManager->SetDevice (ue);
ue->SetCqiManager (cqiManager);
```