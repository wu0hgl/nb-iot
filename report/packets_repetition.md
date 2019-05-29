# 数据包 #

```C++
class Packet {
  double m_timeStamp;
  int m_size;
  int m_id;				// m_uid = Simulator::Init()->GetUID ();
  PacketTAGs 	*m_tags;	// 包分类

  UDPHeader		*m_UDPHeader;
  IPHeader		*m_IPHeader;
  PDCPHeader	*m_PDCPHeader;
  RLCHeader		*m_RLCHeader;
  MACHeader		*m_MACHeader;
}

class PacketTAGs {
  ApplicationType m_type;	// 应用层类型
  int m_applicationSize;	// 应用层大小

  //for video flows
  int m_frameNumber;
  int m_startByte;
  int m_endBytes;
}

void
CBR::Send (void)
{
  //CREATE A NEW PACKET (ADDING UDP, IP and PDCP HEADERS)
  Packet *packet = new Packet ();
  int uid = Simulator::Init()->GetUID ();

  packet->SetID(uid);
  packet->SetTimeStamp (Simulator::Init()->Now ());
  packet->SetSize (GetSize ());					// 包大小128

  PacketTAGs *tags = new PacketTAGs ();
  tags->SetApplicationType(PacketTAGs::APPLICATION_TYPE_CBR);
  tags->SetApplicationSize (packet->GetSize ());
  packet->SetPacketTags(tags);


  UDPHeader *udp = new UDPHeader (GetClassifierParameters ()->GetSourcePort (),
                                  GetClassifierParameters ()->GetDestinationPort ());
  packet->AddUDPHeader (udp);					// 128 + 8 = 136

  IPHeader *ip = new IPHeader (GetClassifierParameters ()->GetSourceID (),
                               GetClassifierParameters ()->GetDestinationID ());
  packet->AddIPHeader (ip);						// 136 + 20 = 156

  PDCPHeader *pdcp = new PDCPHeader ();
  packet->AddPDCPHeader (pdcp);					// 156 - 28 + 3 = 131, 压缩算法ROHC, 131 + 2 = 133

  Trace (packet);

  GetRadioBearer()->Enqueue (packet);			// 发送MacQueue插入, 无线承载发送

  ScheduleTransmit (GetInterval ());

}
```

# 无线承载 #

```C++
void
RadioBearer::Enqueue (Packet *packet)
{
DEBUG_LOG_START_1(LTE_SIM_TEST_ENQUEUE_PACKETS)
  cout << "Enqueue packet on " << GetSource ()->GetIDNetworkNode () << endl;
DEBUG_LOG_END

  GetMacQueue ()->Enqueue(packet);

  // send scheduling request if the source is a UE
  NetworkNode* src = GetSource();
  if (src->GetNodeType() == NetworkNode::TYPE_UE)
    {
      UserEquipment* ue = (UserEquipment*)src;

/*
 *******************发送调度请求*********************
*/      
      ue->GetMacEntity()->SendSchedulingRequest();			
    }
}
```

数据插入mac层队列
```C++
bool
MacQueue::Enqueue (Packet *packet)
{
  QueueElement element (packet);
  GetPacketQueue ()->push_back(element);

  UpdateQueueSize (element.GetSize ());
  UpdateNbDataPackets ();

  return true;
}

class MacQueue
{
private:
  struct QueueElement
  {    
    Packet *m_packet;
    bool m_fragmentation;
    int m_fragmentNumber;		// 分割包的id
    int m_fragmentOffset;		// 数据包分隔后的偏移

    //Can be used for HARQ process
    bool m_lastFragment;
    int m_tempFragmentNumber;
    int m_tempFragmentOffset;
  }
  typedef deque<QueueElement> PacketQueue;
  PacketQueue *m_queue;
  int m_maxSize;		//XXX NOT IMPLEMENTED
  int m_queueSize;
  int m_nbDataPackets;
}

```

# 发送调度请求 #

```C++
void
UeMacEntity::SendSchedulingRequest ()
{

  CheckForDropPackets ();			// 检查丢弃数据包

  //compute the buffer status report
  int bufferStaturReport = 0;
  RrcEntity *rrc = GetDevice ()->GetProtocolStack ()->GetRrcEntity ();

  if (rrc->GetRadioBearerContainer ()->size() > 0)
    {
      for (auto bearer : *rrc->GetRadioBearerContainer ())
        {
          if (bearer->GetApplication()->GetApplicationType ()
              != Application::APPLICATION_TYPE_INFINITE_BUFFER)
            {
              bufferStaturReport += bearer->GetQueueSize ();		// RLC (2 bytes) and MAC (3 bytes) headers and CRC (3 bytes), macqueue的大小不变, 还是133
            }
          else
            {
              bufferStaturReport += 10000000;
            }
        }
    }

  UserEquipment* thisNode = GetDevice ();
  //send the message
  if(thisNode->GetNodeState() == NetworkNode::STATE_ACTIVE)			// ue接收到msg4时, 才会发送
    {
      //create the message
      SchedulingRequestIdealControlMessage *msg = new SchedulingRequestIdealControlMessage ();
      msg->SetSourceDevice (thisNode);
      msg->SetDestinationDevice (thisNode->GetTargetNode ());
      msg->SetBufferStatusReport (bufferStaturReport);
      GetDevice ()->GetPhy ()->SendIdealControlMessage (msg);
      thisNode ->SetLastActivity();
    }
}

void
UeMacEntity::CheckForDropPackets (void)
{
  RrcEntity *rrc = GetDevice ()->GetProtocolStack ()->GetRrcEntity ();

  for (auto bearer : *rrc->GetRadioBearerContainer())
	{
	  if (bearer->GetMacQueue()->GetNbDataPackets()>0)						// 检查macqueue丢弃数据包
		{
		  //delete packets from queue
		  bearer->GetMacQueue ()->CheckForDropPackets (
		  bearer->GetQoSParameters ()->GetMaxDelay (), bearer->GetApplication ()->GetApplicationID ());
		}

	  //delete fragment waiting in AM RLC entity
	  if (bearer->GetRlcEntity()->GetRlcModel() == RlcEntity::AM_RLC_MODE)	// amrlc
		{
		  AmRlcEntity* amRlc = (AmRlcEntity*) bearer->GetRlcEntity();
		  amRlc->CheckForDropPackets (
			bearer->GetQoSParameters ()->GetMaxDelay (), bearer->GetApplication ()->GetApplicationID ());
		}
	}
}
```


# AmRlc #

```C++
class AmRlcEntity: public RlcEntity {
private:
  AmStateVariables* m_amStateVariables;		// 各种记录
  vector<AmdRecord*> *m_sentAMDs;			// 发送记录
  vector <AmdRecord*> *m_receivedAMDs;		// 接收记录
};

class AmdRecord
{
public:
  AmdRecord (Packet* p, int sn);
  virtual ~AmdRecord();

  Packet* m_packet;
  int m_sn; //sequence number
  int m_retx_count;

  bool m_toRetransmit;
};
```

# 控制信息 #

```C++
class IdealControlMessage {
private:
  NetworkNode* m_source;
  NetworkNode* m_destination;
  MessageType m_type;
}

class SchedulingRequestIdealControlMessage : public IdealControlMessage {
private:
  int m_bufferStatusReport;
};
```

# 设备信息 #

UE
```C++
class UserEquipment : public NetworkNode
{
private:
  ENodeB* m_targetNode;
  ENodeB::UserEquipmentRecord* m_targetNodeRecord;
  CqiManager *m_cqiManager;

  bool m_isIndoor;

  double m_timePositionUpdate;
  double m_activityTimeout;
  shared_ptr<Event> m_activityTimeoutEvent;
};
```

eNB
```C++
class ENodeB : public NetworkNode
{
public:
  struct UserEquipmentRecord
  {
    	UserEquipment *m_UE;

    bool m_cqiAvailable;

    	vector<int> m_cqiFeedback;

    int m_riFeedback;

    vector< shared_ptr<arma::cx_fmat> > m_channelMatrix;

    vector< vector<int> > m_pmiFeedback;

    	int m_schedulingRequest; // in bytes

    	int m_averageSchedulingGrants; // in bytes

    int m_ulMcs;

    	vector<double> m_uplinkChannelStatusIndicator;

    	int m_DlTxMode;		// 下行天线数量

    	HarqManager* m_harqManager;
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

private:
  UserEquipmentRecords *m_userEquipmentRecords;
};
```

# PacketBurst #
```C++
class PacketBurst
{
private:
  list<Packet* > m_packets;
};
```

# 信号 #
```C++
class TransmittedSignal
{
private:
  vector< vector<double> > m_values; 	//transmitted power for each MIMO path and sub-carrier
  vector< vector<float> > m_phases; 	//phase shift of received signal for each MIMO path and sub-carrier
  bool m_isMBSFNSignal;
};

/*
 * The meaning of the values of TransmittedSignal is: one value per subchannel and per TX antenna;
 * The meaning of the values of ReceivedSignal is: one value per subchannel and per MIMO path;
 * Otherwise, the two classes are the same.
 */
typedef TransmittedSignal ReceivedSignal;
```

# 数据包调度 #

上行调度器
```C++
class nbUplinkPacketScheduler: public UplinkPacketScheduler {
public:
  nbUplinkPacketScheduler();
  nbUplinkPacketScheduler(EnbMacEntity* mac);
  virtual ~nbUplinkPacketScheduler();
  virtual double ComputeSchedulingMetric (UserToSchedule* user, int subchannel){return 0;};

  struct UserToSchedule
    {
      UserEquipment* 	m_userToSchedule;
      int 				m_transmittedData; //bytes
      int 				m_dataToTransmit; //bytes
      double 			m_averageSchedulingGrant; // in bytes
      int 				m_subcarrier;

      vector<int> 		m_listOfAllocatedRUs;
      int 				m_selectedMCS;
    };

  typedef vector<UserToSchedule*> UsersToSchedule;

private:
  UsersToSchedule* 			m_usersToSchedule;
  vector< vector<int> > 	m_RUmap = vector< vector<int> >(5);
  vector<UserToSchedule> 	m_users;
};
```

轮询调度器
```C++
class nbRoundRobinUplinkPacketScheduler : public nbUplinkPacketScheduler {		--------> 加重传
private:
  int 	m_roundRobinId;
  vector< vector<int> > 	m_RUmap = vector< vector<int> >(5);		// 资源映射
  vector<UserToSchedule> 	m_users;								// 
  vector<UserToSchedule>  	m_queue;

	m_RUmap[0][idx] = id;
	m_RUmap[1][idx] = nru;
	m_RUmap[2][idx] = mcs;
	m_RUmap[3][idx] = tbs;
	m_RUmap[4][idx] = nru;
};
```

# 包组分隔 #

```C++
void
UeMacEntity::ScheduleNbUplinkTransmission (int mcs, int ru)
{
  int availableBytes = (GetNbAmcModule()->GetTBSizeFromMCS(mcs, ru))/8;
DEBUG_LOG_START_1(LTE_SIM_SCHEDULER_DEBUG)
  cout << "SCHEDULE UL TRANSMISSION UE " << GetDevice()->GetIDNetworkNode()
       << " BYTE " << availableBytes
       << endl;
DEBUG_LOG_END
  shared_ptr<PacketBurst> pb = make_shared<PacketBurst> ();
  RrcEntity *rrc = GetDevice ()->GetProtocolStack ()->GetRrcEntity ();

  if (rrc->GetRadioBearerContainer ()->size() > 0)
    {
      for ( auto bearer : *rrc->GetRadioBearerContainer () )
        {
          if (availableBytes > 0)
            {
              RlcEntity *rlc = bearer->GetRlcEntity ();
              shared_ptr<PacketBurst> pb2 = rlc->TransmissionProcedure (availableBytes);
              if (pb2->GetNPackets () > 0)
                {
                  for (auto packet : pb2->GetPackets ())
                    {
                      pb->AddPacket (packet->Copy ());
                    }
                }
              availableBytes -= pb2->GetSize ();
            }
        }
      GetDevice ()->SendPacketBurst (pb);
    }
}
```

# 首部 #

```C++
class Header
{
public:
  enum ProtocolHeader
  {
    HEADER_UDP,
    HEADER_TCP,
    HEADER_IP,
    HEADER_PDCP,
    HEADER_RLC,
    HEADER_MAC
  };

private:
  int m_size;
  ProtocolHeader m_protocolHeader;
};


/*
 * TRANSPORT PROTOCOL HEADER
 */
class TransportHeader : public Header
{
private:
  int m_sourcePort;
  int m_destinationPort;
  TransportProtocol::TransportProtocolType m_protocolType;
};

/*
 * UDP HEADER
 */
class UDPHeader : public TransportHeader
{
public:
  UDPHeader() = default;
  UDPHeader(int sourcePort, int destinationPort);
  virtual ~UDPHeader() = default;
};



/*
 * IP HEADER
 */
class IPHeader : public Header
{

private:
  int m_sourceID;
  int m_destinationID;
};


/*
 * PDCP HEADER
 */
class PDCPHeader : public Header
{
public:
  PDCPHeader();
  virtual ~PDCPHeader() = default;
};


/*
 * RLC HEADER
 */
class RLCHeader : public Header
{
private:
  bool m_isAFragment;
  bool m_isTheLatestFragment; //Describes that there is a fragment of packet
  int m_fragmentNumber;
  int m_rlcEntityIndex;

  int m_rlcPduSequenceNumber;

  int m_startByte;
  int m_endByte;
};


/*
 * MAC HEADER
 */
class MACHeader : public Header
{

private:
  int m_macSourceID;
  int m_macDestinationID;
};
```

接收数据包的过程
LteChannel::StartRx
EnbLtePhy::StartRx 
NetworkNode::ReceivePacketBurst
AmRlcEntity::ReceptionProcedure 
AmRlcEntity::ReceptionProcedureEnd

发送数据包的过程
```shell
(gdb) bt
#0  LteChannel::StartTx (this=0xda1840, p=std::shared_ptr (count 4, weak 0) 0xe57700, txSignal=0xe10dc0, src=0xda3760)
    at src/channel/LteChannel.cpp:51
#1  0x00000000005467fe in UeLtePhy::StartTx (this=0xda3fc0, p=std::shared_ptr (count 4, weak 0) 0xe57700)
    at src/phy/ue-lte-phy.cpp:123
#2  0x00000000004ec7ff in NetworkNode::SendPacketBurst (this=0xda3760, p=std::shared_ptr (count 4, weak 0) 0xe57700)
    at src/device/NetworkNode.cpp:193
#3  0x0000000000464a5f in UeMacEntity::ScheduleNbUplinkTransmission (this=0xda3c80, mcs=6, ru=10)		------> 涉及amrlc, 包组分隔
    at src/protocolStack/mac/ue-mac-entity.cpp:190
#4  0x000000000054fe31 in UeLtePhy::ReceiveIdealControlMessage (this=0xda3fc0, msg=0xe10c80) at src/phy/ue-lte-phy.cpp:1291
#5  0x000000000057dd4e in EnbLtePhy::SendIdealControlMessage (this=0xda1e40, msg=0xe10c80) at src/phy/enb-lte-phy.cpp:186
#6  0x00000000004ad9c9 in nbUplinkPacketScheduler::DoStopSchedule (this=0xda2ce0)
    at src/protocolStack/mac/packet-scheduler/nb-uplink-packet-scheduler.cpp:209
#7  0x00000000004ad653 in nbUplinkPacketScheduler::DoSchedule (this=0xda2ce0)
    at src/protocolStack/mac/packet-scheduler/nb-uplink-packet-scheduler.cpp:160
#8  0x0000000000494e3d in PacketScheduler::Schedule (this=0xda2ce0) at src/protocolStack/mac/packet-scheduler/packet-scheduler.cpp:72
#9  0x00000000004e14ed in ENodeB::UplinkResourceBlockAllocation (this=0xda1ae0) at src/device/ENodeB.cpp:533
#10 0x00000000004e1498 in ENodeB::ResourceBlocksAllocation (this=0xda1ae0) at src/device/ENodeB.cpp:525
#11 0x00000000005999ab in std::shared_ptr<Event> MakeEvent<void (ENodeB::*)(), ENodeB*>(void (ENodeB::*)(), ENodeB*)::EventMemberImpl0::RunEvent() (this=0xda7cb0) 
	at src/componentManagers/../core/eventScheduler/make-event.h:95
#12 0x00000000005ac9b2 in Simulator::ProcessOneEvent (this=0xda0c70) at src/core/eventScheduler/simulator.cpp:90
#13 0x00000000005ac8e2 in Simulator::Run (this=0xda0c70) at src/core/eventScheduler/simulator.cpp:75
#14 0x000000000060a5ce in nbCell (argc=13, argv=0x7fffffffe3d8) at src/scenarios/nb-cell.h:335
#15 0x0000000000619014 in main (argc=13, argv=0x7fffffffe3d8) at src/5G-simulator.cpp:120
```

# 重传实现 #

```
- ID	2	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- TTI	10	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- MCS	6	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- TBS	125	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- RU	10	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
```

```
- ID	2	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- TTI	10	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- MCS	6	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- TBS	125	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- RU	10	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- repeatID	4	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	

# 传输一个RU之后 TTI重新设置为RU, 重复次数-1
- ID	2	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- TTI	10	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- MCS	6	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- TBS	125	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- RU	10	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
- repeatID	3	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1    -1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	-1	
```

## 重传1 ##

数据包传输时间大于重传时间
```
zyb@server:~/nb_iot$ ./5G-simulator nbCell    1      1500     1    1      5         1        3.75      1        60          128      1

TX CBR ID 0 B 0 SIZE 133 SRC 2 DST 1 T 55.699
RACH INFO SF 56000 WIN 1 COLLISIONS 0 TOT 1
SRC 2 repeatID 4
SRC 2 repeatID 3
SRC 2 repeatID 2
SRC 2 repeatID 1
SRC 2 repeatID 4
SRC 2 repeatID 3
SRC 2 repeatID 2
SRC 2 repeatID 1
RX CBR ID 0 B 0 SIZE 128 SRC 2 DST 1 D 1.965
TX CBR ID 1 B 0 SIZE 133 SRC 2 DST 1 T 115.699
RACH INFO SF 115840 WIN 1 COLLISIONS 0 TOT 1
SRC 2 repeatID 4
SRC 2 repeatID 3
SRC 2 repeatID 2
SRC 2 repeatID 1
SRC 2 repeatID 4
SRC 2 repeatID 3
SRC 2 repeatID 2
SRC 2 repeatID 1
RX CBR ID 1 B 0 SIZE 128 SRC 2 DST 1 D 1.805
```

## 重传1 ##

假设应用层发送数据包时间小于重传所需时间

```
zyb@server:~/nb_iot$ ./5G-simulator nbCell    1      1500     1    1      5         1        3.75      1        2          128      1
TX CBR ID 0 B 0 SIZE 133 SRC 2 DST 1 T 2.823
RACH INFO SF 2880 WIN 1 COLLISIONS 0 TOT 1
SRC 2 repeatID 4
SRC 2 repeatID 3
SRC 2 repeatID 2
SRC 2 repeatID 1
SRC 2 repeatID 4
SRC 2 repeatID 3
SRC 2 repeatID 2
SRC 2 repeatID 1
TX CBR ID 1 B 0 SIZE 133 SRC 2 DST 1 T 4.823
SRC 2 repeatID 4
SRC 2 repeatID 3
SRC 2 repeatID 2
SRC 2 repeatID 1
RX CBR ID 0 B 0 SIZE 128 SRC 2 DST 1 D 3.257
SRC 2 repeatID 4
SRC 2 repeatID 3
SRC 2 repeatID 2
SRC 2 repeatID 1
TX CBR ID 2 B 0 SIZE 133 SRC 2 DST 1 T 6.823
SRC 2 repeatID 4
SRC 2 repeatID 3
SRC 2 repeatID 2
SRC 2 repeatID 1
RX CBR ID 1 B 0 SIZE 128 SRC 2 DST 1 D 3.273
```

```
zyb@server:~/nb_iot$ ./5G-simulator nbCell    1      1500     1    1      5         1        3.75      1        1          128      1 
TX CBR ID 0 B 0 SIZE 133 SRC 2 DST 1 T 1.912		------------> ID 0
RACH INFO SF 1920 WIN 1 COLLISIONS 0 TOT 1
SRC 2 repeatID 4
SRC 2 repeatID 3
SRC 2 repeatID 2
TX CBR ID 1 B 0 SIZE 133 SRC 2 DST 1 T 2.912
SRC 2 repeatID 1
SRC 2 repeatID 4
SRC 2 repeatID 3
TX CBR ID 2 B 0 SIZE 133 SRC 2 DST 1 T 3.912
SRC 2 repeatID 2
SRC 2 repeatID 1									------------> 重传4次后最近一个RX ID 0 
RX CBR ID 0 B 0 SIZE 128 SRC 2 DST 1 D 2.568
SRC 2 repeatID 4
TX CBR ID 3 B 0 SIZE 133 SRC 2 DST 1 T 4.912
SRC 2 repeatID 3
SRC 2 repeatID 2
SRC 2 repeatID 1
RX CBR ID 1 B 0 SIZE 128 SRC 2 DST 1 D 2.848		------------> 重传4次后最近一个RX ID 1
TX CBR ID 4 B 0 SIZE 133 SRC 2 DST 1 T 5.912
SRC 2 repeatID 4
SRC 2 repeatID 3
SRC 2 repeatID 2
TX CBR ID 5 B 0 SIZE 133 SRC 2 DST 1 T 6.912
SRC 2 repeatID 1
RX CBR ID 2 B 0 SIZE 128 SRC 2 DST 1 D 3.128
SRC 2 repeatID 4
SRC 2 repeatID 3
TX CBR ID 6 B 0 SIZE 133 SRC 2 DST 1 T 7.912
SRC 2 repeatID 2
SRC 2 repeatID 1
RX CBR ID 3 B 0 SIZE 128 SRC 2 DST 1 D 3.408
SRC 2 repeatID 4
TX CBR ID 7 B 0 SIZE 133 SRC 2 DST 1 T 8.912
```

## 重传2 ##

多个用户
```
TX CBR ID 0 B 4 SIZE 133 SRC 6 DST 1 T 1.157
RACH INFO SF 1280 WIN 1 COLLISIONS 0 TOT 1
SRC 6 repeatID 4
TX CBR ID 1 B 1 SIZE 133 SRC 3 DST 1 T 1.554
TX CBR ID 2 B 3 SIZE 133 SRC 5 DST 1 T 1.607
RACH INFO SF 1600 WIN 1 COLLISIONS 0 TOT 1
SRC 6 repeatID 3
TX CBR ID 3 B 0 SIZE 133 SRC 2 DST 1 T 1.912
SRC 3 repeatID 4
RACH INFO SF 1920 WIN 2 COLLISIONS 0 TOT 2
TX CBR ID 4 B 2 SIZE 133 SRC 4 DST 1 T 1.952
SRC 6 repeatID 2
TX CBR ID 5 B 4 SIZE 133 SRC 6 DST 1 T 2.157
SRC 3 repeatID 3
SRC 2 repeatID 4
SRC 5 repeatID 4
RACH INFO SF 2240 WIN 1 COLLISIONS 0 TOT 1
SRC 6 repeatID 1
RX CBR ID 0 B 4 SIZE 128 SRC 6 DST 1 D 1.147
SRC 6 repeatID 4
TX CBR ID 6 B 1 SIZE 133 SRC 3 DST 1 T 2.554
SRC 3 repeatID 2
SRC 2 repeatID 3
SRC 5 repeatID 3
SRC 4 repeatID 4
TX CBR ID 7 B 3 SIZE 133 SRC 5 DST 1 T 2.607
SRC 6 repeatID 3
SRC 6 repeatID 2
SRC 3 repeatID 1
SRC 2 repeatID 2
SRC 5 repeatID 2
SRC 4 repeatID 3
TX CBR ID 8 B 0 SIZE 133 SRC 2 DST 1 T 2.912
SRC 6 repeatID 1
TX CBR ID 9 B 2 SIZE 133 SRC 4 DST 1 T 2.952
TX CBR ID 10 B 4 SIZE 133 SRC 6 DST 1 T 3.157
SRC 3 repeatID 4
SRC 2 repeatID 1
SRC 5 repeatID 1
SRC 4 repeatID 2
SRC 6 repeatID 4
SRC 3 repeatID 3
SRC 2 repeatID 4
SRC 5 repeatID 4
SRC 4 repeatID 1
RX CBR ID 4 B 2 SIZE 128 SRC 4 DST 1 D 1.568
```


```C++
class MacQueue
{
private:
  struct QueueElement
  {
    Packet *m_packet;
    bool m_fragmentation;       // 是否是一个片段
    int m_fragmentNumber;   
    int m_fragmentOffset;

    //Can be used for HARQ process
    bool m_lastFragment;
    int m_tempFragmentNumber;
    int m_tempFragmentOffset;

    Packet*
    GetPacket(void);
  };

  typedef deque<QueueElement> PacketQueue;
  PacketQueue *m_queue;
  int m_maxSize;
  int m_queueSize;
  int m_nbDataPackets;
};
```

数据包分隔, 数据包编号从0开始, rlc编号从1开始
```C++
Packet*
MacQueue::GetPacketToTramsit (int availableBytes)
{
  int overhead = 8; //RLC + MAC + CRC overhead
  if (IsEmpty () || overhead >= availableBytes)
    {
      return nullptr;
    }

  QueueElement element = Peek();        // 数据包队列第一个

  RLCHeader *rlcHeader = new RLCHeader ();

  int dataToSend;
  int fragmentSize = 0;

  if (element.GetFragmentation())       // 数据包是一个片段, rlc首部从上次偏移量开始
    {
      dataToSend = element.GetSize() - element.GetFragmentOffset ();
      rlcHeader->SetAFragment(true);
      rlcHeader->SetTheLatestFragment (true);
      rlcHeader->SetStartByte (element.GetFragmentOffset ());
    }
  else                                  // 数据包不是片段, rlc首部从0开始
    {
      dataToSend = element.GetSize ();
      rlcHeader->SetStartByte (0);
    }

  Packet *packet = GetPacketQueue ()->begin ()->GetPacket ()->Copy();   // 数据包拷贝

  //CASE 1 --> PACKET FRAGMENTATION
  if(dataToSend + overhead > availableBytes)
    {
      fragmentSize = availableBytes - overhead;
      packet->SetSize(fragmentSize);

      GetPacketQueue ()->begin ()->SetFragmentOffset (fragmentSize);
      GetPacketQueue ()->begin ()->SetFragmentation (true);
      GetPacketQueue ()->begin ()->SetFragmentNumber (element.GetFragmentNumber () + 1);    // 数据包编号从1开始

      rlcHeader->SetAFragment (true);
      rlcHeader->SetTheLatestFragment (false);
      rlcHeader->SetFragmentNumber (element.GetFragmentNumber ());
      rlcHeader->SetEndByte (rlcHeader->GetStartByte () + fragmentSize - 1);

      UpdateQueueSize (-fragmentSize);
    }
  // CASE 2 -> NO other PACKET FRAGMENTATION
  else
    {
      rlcHeader->SetFragmentNumber (element.GetFragmentNumber ());
      rlcHeader->SetEndByte (element.GetSize () - 1);
      Dequeue ();
      UpdateQueueSize (-dataToSend);
      packet->SetSize(dataToSend);
    }

  packet->AddRLCHeader(rlcHeader);
  return packet;
}
```


# am rlc层 #

首先检查`vector<AmdRecord*> *m_sentAMDs;`是否有值, m_sentAMDs.size()=0为新传
```
class AmdRecord {
public:
  Packet* m_packet;  // 
  int m_sn;          //sequence number
  int m_retx_count;  // 重复计数

  bool m_toRetransmit;
};
```
从MacQueue层拿出一个包进行传输