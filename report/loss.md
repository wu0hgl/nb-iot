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


# shadowing #

[double ChannelRealization::GetShadowing (void)](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/channel/propagation-model/channel-realization.cpp#L750)


[void ChannelRealization::ShortTermUpdate(void)](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/channel/propagation-model/channel-realization.cpp#L327)


# penetrationlLoss #

[double ChannelRealization::GetPenetrationLoss (void)](https://github.com/wu0hgl/nb-iot/blob/a7907b552f28821ae84cd063763490e68bb0c6b7/src/channel/propagation-model/channel-realization.cpp#L775)


# 修改函数 #

[channel-realization.cpp](https://github.com/wu0hgl/nb-iot/blob/master/src/channel/propagation-model/channel-realization.cpp)