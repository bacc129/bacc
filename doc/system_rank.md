# 赛季比赛说明

## 奖励存放账户

赛季比赛的奖励会直接转入以下账户中:

- 投票币龄排行榜奖励账户 rank.all
- 新增投票币龄排行榜奖励账户 rank.new
- 邀请币龄排行榜奖励账户 rank.invite

## 获取排行信息

链会记录所有排行数据, 可以直接从链上取得排行榜, 根据排名可以发放奖励.

其中所有投票排行榜都记录在noc账户下的voterank表中, 根据不同的名称可以查询:

- 投票币龄排行榜奖励 rank.all
- 新增投票币龄排行榜奖励 rank.new
- 邀请币龄排行榜奖励 rank.invite

投票币龄排行榜奖励:

```bash
./nocli -u http://127.0.0.1:8001 get table noc rank.all voterank
```

新增投票币龄排行榜奖励:

```bash
./nocli -u http://127.0.0.1:8001 get table noc rank.new voterank
```

邀请币龄排行榜奖励:

```bash
./nocli -u http://127.0.0.1:8001 get table noc rank.invite voterank
```

注意币龄的计算请参照投票文档中的方法.
