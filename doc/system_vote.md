# 投票相关操作

## 1. 抵押Token以换取票权

要投票需要先抵押Token以换取投票权.

通过noc的freezed表可以获得用户抵押Token的信息:

```bash
./nocli -u http://127.0.0.1:8001 get table bacc bacc freezed -L test.a
{
  "rows": [{
      "staked": "20000.0000 BACC",
      "unstaking": "0.0000 BACC",
      "voter": "test.a",
      "unstake_height": 12378
    }
  ],
  "more": false
}
```

这里抵押token的每个用户都有一项:

- staked 当前抵押的token, 这也意味着用户可以投多少票, 用户投票会减少这里的指
- unstaking 当前处于赎回中的token
- voter 用户名
- unstake_height 如果unstaking>0, 则为赎回token的解锁块高度

通过freeze可以抵押:

```bash
./nocli -u http://127.0.0.1:8001 push action bacc freeze '{"voter":"test.a","stake":"20000.0000 BACC"}' -p test.a
```

注意这里stake是指用户抵押的总量, 如果执行时stake大于当前抵押量,则为追加差值, 如果小于,则为申请赎回抵押代币.
也就是说如果要赎回所有代币stake即为0.

当unstaking大于0,切unstake_height已经到达时, 可以赎回token:

```bash
./nocli -u http://127.0.0.1:8001 push action bacc unfreeze '{"voter":"test.a"}' -p test.a
```

## 2. 活期投票

```bash
./nocli -u http://127.0.0.1:8001 get table bacc test.a votes
{
  "rows": [{
      "bpname": "test.d",
      "vote": "5.0000 BACC",
      "votepower": "1.0000 BACC",
      "voteage": 126000,
      "voteage_update_height": 12966,
      "static_votes": []
    },{
      "bpname": "test.e",
      "vote": "5.0000 BACC",
      "votepower": "1.0000 BACC",
      "voteage": 0,
      "voteage_update_height": 13242,
      "static_votes": [{
          "vote": "800.0000 BACC",
          "votepower": "1600.0000 BACC",
          "votepower_age": 0,
          "votepower_age_update_height": 13385,
          "unfreeze_block_num": 8134985,
          "freeze_block_num": 8121600,
          "is_has_quited": 0
        },{
          "vote": "800.0000 BACC",
          "votepower": "1600.0000 BACC",
          "votepower_age": 0,
          "votepower_age_update_height": 13473,
          "unfreeze_block_num": 8135073,
          "freeze_block_num": 8121600,
          "is_has_quited": 0
        }
      ]
    }
  ],
  "more": false
}
```

投票信息比较复杂, 我们这里分开看:

首先列表中每一项对应一个账户对一个bp的投票, 用户领取分红针对的是其对一个bp投票获得的所有分红,
投票信息分两种:活期投票和定期锁定投票

活期投票信息直接在每一项中, 定期锁定投票在static_votes数组中, 每一项都是一个定期投票:

活期投票:

- vote : 活期投票token数量
- votepower : 活期投票加权值, 这个对于节点来说代表实际能购获得多少票
- voteage : 当前已结算币龄
- voteage_update_height : 当前已结算加权币龄时的块高度

定期锁定投票:

定期锁定投票对于一个节点同样的时间也会有很多不能合并的项, 所以会返回一个数组:

- vote : 活期投票token数量
- votepower : 活期投票加权值, 这个对于节点来说代表实际能购获得多少票
- votepower_age : 当前已结算加权币龄
- votepower_age_update_height : 当前已结算加权币龄时的块高度
- unfreeze_block_num : 定期截止块高度
- freeze_block_num : 定期开始块高度
- is_has_quited : 是否已经失效, 这个为ture时该条票权数据忽略不计, 系统会在用户领取剩余奖励之后清除.

注意这里的票龄数据:

在NOC中票龄 = 加权投票数 * 块高度间隔, 分红是根据票龄比例来分配的,
在涉及到票龄的处理中会有以下三个变量:

- 投票加权值
- 当前已结算加权币龄
- 当前已结算加权币龄时的块高度

系统在处理 `投票加权值` 变更时会结算当前的票龄 将其存入 `当前已结算加权币龄` 中, 并同时记录 此时的块高度 到 `当前已结算加权币龄时的块高度` 中.

这意味着 票龄 = 投票加权值 * (当前块高度 - 当前已结算加权币龄时的块高度) + 当前已结算加权币龄

所以对于 活期投票, 此时用户票龄 = votepower * ( current_block_num - voteage_update_height ) + voteage.

下面是具体的行为:

用户活期投票:

```bash
./nocli -u http://127.0.0.1:8001 push action bacc vote '{"voter":"test.a","bpname":"test.d","stake":"1500.0000 BACC"}' -p test.a
```

- voter : 投票用户
- bpname : 要投票的bp
- stake : 投票token

注意, 在投票之前需要锁定足够的token以换取票权.

执行成功则可以查到投票信息

```bash
./nocli -u http://127.0.0.1:8001 get table bacc test.a votes
{
  "rows": [{
      "bpname": "test.d",
      "vote": "5.0000 BACC",
      "votepower": "1.0000 BACC",
      "voteage": 126000,
      "voteage_update_height": 12966,
      "static_votes": []
    }
  ],
  "more": false
}
```

需要注意的是, 这里的stake指当前用户对节点的全部投票数, 也就是说, 以1000 NOC调用后, 再以2000 NOC调用, 则投票数为2000 BACC,
也就是说, 如果撤回所有投票的话, stake则为 0.0000 BACC

## 3. 定期锁定投票

定期锁定投票有以下几个档次:

```cpp
    const static auto static_vote_data = vector<std::pair<uint32_t, uint64_t>>{
        { 21,   1 },
        { 47,   2 },
        { 97,   4 },
        { 198,  7 },
        { 297, 10 },
    };
```

对应的是定期锁定投票的typ:

```bash
./nocli -u http://127.0.0.1:8001 push action bacc votestatic '{"voter":"test.a","bpname":"test.e","stake":"800.0000 BACC","typ":1}' -p test.a
```

- voter : 投票用户
- bpname : 要投票的bp
- stake : 投票token
- typ : 定期类型, 1 代表 21 天, 2 代表 47 天, 3 代表 97 天, 4 代表 198 天, 5 代表 297 天

成功之后可以查询

```bash
./nocli -u http://127.0.0.1:8001 get table bacc test.a votes
{
  "rows": [{
      "bpname": "test.d",
      "vote": "5.0000 BACC",
      "votepower": "1.0000 BACC",
      "voteage": 126000,
      "voteage_update_height": 12966,
      "static_votes": []
    },{
      "bpname": "test.e",
      "vote": "5.0000 BACC",
      "votepower": "1.0000 BACC",
      "voteage": 0,
      "voteage_update_height": 13242,
      "static_votes": [{
          "vote": "800.0000 BACC",
          "votepower": "1600.0000 BACC",
          "votepower_age": 0,
          "votepower_age_update_height": 13385,
          "unfreeze_block_num": 8134985,
          "freeze_block_num": 8121600,
          "is_has_quited": 0
        }
      ]
    }
  ],
  "more": false
}
```

当定期投票到期之后, 用户需要手动赎回其定期投票:

```bash
./nocli -u http://127.0.0.1:8001 push action bacc quitsvote '{"voter":"test.a","bpname":"bacc.initbpa","idx":1}' -p test.a
```

需要注意的是这里面的idx, 这个值是static_votes数组中需要赎回的定期投票的位置, 如果其为第一个, 则idx为0, 如果是第二个, 则为1, 以此类推.

如下面的投票:

```json
{
  "rows": [{
      "bpname": "test.d",
      "vote": "5.0000 BACC",
      "votepower": "1.0000 BACC",
      "voteage": 126000,
      "voteage_update_height": 12966,
      "static_votes": []
    },{
      "bpname": "test.e",
      "vote": "5.0000 BACC",
      "votepower": "1.0000 BACC",
      "voteage": 0,
      "voteage_update_height": 13242,
      "static_votes": [{
          "vote": "800.0000 BACC",
          "votepower": "1600.0000 BACC",
          "votepower_age": 0,
          "votepower_age_update_height": 13385,
          "unfreeze_block_num": 8134985,
          "freeze_block_num": 8121600,
          "is_has_quited": 0
        },{
          "vote": "1800.0000 BACC",
          "votepower": "3600.0000 BACC",
          "votepower_age": 0,
          "votepower_age_update_height": 13473,
          "unfreeze_block_num": 8135073,
          "freeze_block_num": 8121600,
          "is_has_quited": 0
        }
      ]
    }
  ],
  "more": false
}
```

要赎回"1800.0000 BACC"的那份投票, 则idx填1

## 4. 领取投票分红

这里用户一次将会领取包括活期定期在内所有一个bp所产生的分红:

```bash
./nocli -u http://127.0.0.1:8001 push action bacc claim '{"voter":"test.a","bpname":"bacc.initbpa"}' -p test.a
```
