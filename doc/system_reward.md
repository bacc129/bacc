# 出块奖励相关说明

## 出块奖励分配说明

系统在每个区块会增发NOC作为奖励:

奖励会基于比例直接转账给以下账户:

- 开发者账户 noc.dev
- 生态基金账户 noc.better
- 共振池账户 noc.vpool
- 投票币龄排行榜奖励账户 rank.all
- 新增投票币龄排行榜奖励账户 rank.new
- 邀请币龄排行榜奖励账户 rank.invite

可以查询这些账户的代币余额来判定奖励的多少:

如查询noc.init的余额:

```bash
./nocli -u http://127.0.0.1:8001 get currency balance noc.token noc.init BACC
```

如果想知道某个时间内奖励的增量, 有两种方式:

首先基于区块运行的结果, 编写链插件统计, 这里不推荐

其次是记录一个块高度和余额的数据, 下次查询时计算差值即可.

## 链上部分配置说明

void setvidparam( uint64_t coefficient, asset reward_per_vid );

使用noc账户可以设置vid奖励计算的参数:

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" push action noc setvidparam '{"coefficient":100,"reward_per_vid":"1.0000 BACC"}' -p noc
```

这里的参数存在 vidparam 表中:

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" get table noc noc vidparam
```

这里 coefficient 是奖励系数, reward_per_vid是每个vid的奖励, 每个周期内的奖励为:

```cpp
reward_to_vid = itr->reward_per_vid * itr->vid_reward_coefficient * vs_itr->vid_count;
```

即

```cpp
reward_to_vid = reward_per_vid * vid_reward_coefficient * vid_count;
```
