# 节点相关操作

在节点白名单账户可以注册为节点,
要可以被投票需要质押NOC Token, 赎回抵押资产需要撤票为零才行,
节点收入达到1万个币可以领取，手续费1%，直接到生态基金.

## 0. 白名单管理

白名单在noc下的bpopened表中, 可以查询:

```bash
./nocli -u http://127.0.0.1:8001 get table noc noc bpopened
{
  "rows": [{
      "bpname": "aaaabp"
    },{
      "bpname": "aaaabp2"
    },{
      "bpname": "aaaabp3"
    }
  ],
  "more": false
}
```

表中的账户可以注册为bp.

使用`openbp`可以添加账户名到白名单中:

```bash
./nocli -u http://127.0.0.1:8001 push action noc openbp '{"producer":"account"}' -p noc
```

使用banbp可以将账户名从白名单中删除, 注意这个不会影响已经注册的bp.

```bash
./nocli -u http://127.0.0.1:8001 push action noc banbp '{"producer":"account"}' -p noc
```

## 1. 注册节点

所有节点相关信息都在noc的bps表中, 可以查询:

```bash
./nocli -u http://127.0.0.1:8001 get table noc noc bps
```

```json
   {
      "name": "noc.initbpa",
      "invite": "",
      "block_signing_key": "NOC7R82SaGaJubv23GwXHyKT4qDCVXi66qkQrnjwmBUvdA4dyzEPG",
      "total_staked": 1000000000,
      "rewards_pool": "2996.8596 BACC",
      "rewards_block": "515.9907 BACC",
      "total_voteage": 0,
      "voteage_update_height": 0,
      "url": "",
      "emergency": 0,
      "isactive": 1,
      "pawn": "0.0000 BACC"
   }
```

表中意义:

- name bp名
- invite bp邀请节点名
- block_signing_key bp出块私钥,用于出块签名
- total_staked bp总的得票数
- rewards_pool 分红池
- rewards_block 节点出块奖励
- total_voteage 总票龄
- voteage_update_height 票龄更新块高度
- url url信息
- pawn 节点抵押

可以通过updatebp来注册节点, 也可以通过同样的命令修改bp信息:

```bash
./nocli -u http://127.0.0.1:8001 push action noc updatebp '{"bpname":"test.f","invite":"","block_signing_key":"NOC7R82SaGaJubv23GwXHyKT4qDCVXi66qkQrnjwmBUvdA4dyzEPG","url":""}' -p test.f
```

修改信息:

```bash
./nocli -u http://127.0.0.1:8001 push action noc updatebp '{"bpname":"test.f","invite":"","block_signing_key":"NOC7R82SaGaJubv23GwXHyKT4qDCVXi66qkQrnjwmBUvdA4dyzEPG","url":"newurl"}' -p test.f
```

## 2. 节点质押与赎回

节点需要质押token才能获得投票:

质押token:

```bash
./nocli -u http://127.0.0.1:8001 push action noc bppawn '{"bpname":"test.c","pawn":"200000.0000 BACC"}' -p test.c
```

赎回质押, 在此之前要保证bp没有得到投票:

```bash
./nocli -u http://127.0.0.1:8001 push action noc bpredeem '{"bpname":"noc.initbpa"}' -p noc.initbpa
```

## 3. 节点领取奖励

节点可以通过此领取其奖励:

```bash
./nocli -u http://127.0.0.1:8001 push action noc bpclaim '{"bpname":"noc.initbpa"}' -p noc.initbpa
```
