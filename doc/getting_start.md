# BACC Chain

## Test Chain

测试网 http RPC URL : http://47.111.173.249:8001

P2P Address 47.111.173.249:9001

可以链接 http://47.111.173.249:8001/v1/chain/get_info 获取信息

## 说明

NOC基于FORCEIO项目, 其底层技术基于EOSIO, 在阅读这里的文档之前, 需要阅读EOSIO的开发者文档以建立对于链基本的认识:

https://developers.eos.io/eosio-home/docs

## Genesis Accounts

当前测试网所有创世账户公钥均为测试用的公钥:

- `NOC5muUziYrETi5b6G2Ev91dCBrEm3qir7PK4S2qSFqfqcmouyzCr`
- `5KYQvUwt6vMpLJxqg4jSQNkuRfktDHtYDp8LPoBpYo8emvS1GfG`

所有创世BP签名公钥:

- `NOC7R82SaGaJubv23GwXHyKT4qDCVXi66qkQrnjwmBUvdA4dyzEPG`
- `5JfjatHRwbmY8SfptFRxHnYUctfnuaxANTGDYUtkfrrBDgkh3hB`

注意所有NOC的私钥以NOC开头

测试网创世账户如下:

- 预挖账户 bacc.init
- 开发者账户 bacc.dev
- 生态基金账户 bacc.better
- 共振池账户 bacc.vpool
- BP抵押代币账户 bacc.pawn
- 配置账户 bacc.config
- 投票币龄排行榜奖励账户 rank.all
- 新增投票币龄排行榜奖励账户 rank.new
- 邀请币龄排行榜奖励账户 rank.invite
- 测试用账户(测试用1000万NOC) bacc.test
- 测试用账户(测试用1万NOC)  test.a, test.b, ..., test.t
- 创世BP bacc.initbpa, bacc.initbpb, ..., bacc.initbpq

此外系统账户为 bacc.

系统通证合约为 bacc.token.

## Use Wallet and Cli

终端钱包: /build/bin/nocwallet 这个相当于EOS的keosd
终端工具: /build/bin/nocil 这个相当于EOS的cleos

与EOS终端工具使用类似.

nocwallet为transaction提供根据私钥的签名服务,
nocwallet会提供http接口来为nocil工具提供服务, 常用的命令行参数如下:

```bash
./nocwallet -h
Application Options:

Config Options for eosio::http_plugin:
  --http-server-address arg             The local IP and port to listen for
                                        incoming http connections; leave blank
                                        to disable.
```

要绑定http接口可以使用http-server-address参数:

```bash
./nocwallet --http-server-address 127.0.0.1:3333
```

这时会绑定服务到http://127.0.0.1:3333.

使用nocil加上 --wallet-url 参数可以与其交互:

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" wallet ${SUBCMDS}
```

注意--wallet-dir 参数, 这是钱包数据存放的位置.

所有钱包相关的命令都在wallet命令下, 子命令如下:

- create:    Create a new wallet locally
- open:      Open an existing wallet
- lock:      Lock wallet
- lock_all:  Lock all unlocked wallets
- unlock:    Unlock wallet
- import:    Import private key into wallet
- remove_key:Remove key from wallet
- create_key:Create private key within wallet
- list:      List opened wallets, * = unlocked
- keys:      List of public keys from all unlocked wallets.
- private_keys:                List of private keys from an unlocked wallet in wif or PVT_R1 format.
- stop:      Stop keosd (doesn't work with nodeos).

首先需要创建钱包:

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" wallet create --file ./password
```

这里会生成随机密码, 存在./password文件中. 解锁钱包需要使用这个密码.

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" wallet list
Wallets:
[
  "default *"
]
```

这里可以看到新创建的钱包,名字是default, 可以使用 lock和unlock锁定与解锁:

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" wallet lock
Locked: default
```

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" wallet unlock
password: Unlocked: default
```

注意钱包会在一定时间之后自动锁定, 使用钱包签名必须解锁

可以利用如下命令创建一对公私钥, 并存入文件中:

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" create key --file ./keys
```

可以在文件中查看:

```bash
cat ./keys
Private key: 5KNJ71gt2XSsV693MaWBtLCmCvGPTMLSdj8iHw49XZEcVuwVww3
Public key: NOC7t9gFqCHkzTHukteR5rBxMzrL5yJQBCRKdp7aNt4PqtDykcBwJ
```

使用import可以导入已知的私钥:

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" wallet import --private-key 5KNJ71gt2XSsV693MaWBtLCmCvGPTMLSdj8iHw49XZEcVuwVww3
imported private key for: NOC7t9gFqCHkzTHukteR5rBxMzrL5yJQBCRKdp7aNt4PqtDykcBwJ
```

导入之后可以用keys命令查看当前钱包中所有已知私钥的公钥

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" wallet keys
[
  "NOC7t9gFqCHkzTHukteR5rBxMzrL5yJQBCRKdp7aNt4PqtDykcBwJ"
]
```

在测试网中, 测试账户的私钥都为5KYQvUwt6vMpLJxqg4jSQNkuRfktDHtYDp8LPoBpYo8emvS1GfG, 可以导入:

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" wallet import --private-key 5KYQvUwt6vMpLJxqg4jSQNkuRfktDHtYDp8LPoBpYo8emvS1GfG
imported private key for: NOC5muUziYrETi5b6G2Ev91dCBrEm3qir7PK4S2qSFqfqcmouyzCr
```

这时已经导入两个私钥了:

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" wallet keys
[
  "NOC5muUziYrETi5b6G2Ev91dCBrEm3qir7PK4S2qSFqfqcmouyzCr",
  "NOC7t9gFqCHkzTHukteR5rBxMzrL5yJQBCRKdp7aNt4PqtDykcBwJ"
]
```

这里我们就可以使用测试账号创建一个新账户.

```bash
./nocli --url "http://47.111.173.249:8001" --wallet-url "http://127.0.0.1:3333" create account bacc.test newaccount NOC7t9gFqCHkzTHukteR5rBxMzrL5yJQBCRKdp7aNt4PqtDykcBwJ NOC7t9gFqCHkzTHukteR5rBxMzrL5yJQBCRKdp7aNt4PqtDykcBwJ vida
executed transaction: 09385ce52efb4eed625dbd0571c6c33f6ea7788e764ebd32f8742a733d8d6eb2  208 bytes  593 us
#           bacc <= bacc::newaccount              {"creator":"bacc.test","name":"newaccount","owner":{"threshold":1,"keys":[{"key":"NOC7t9gFqCHkzTHukte...
>> new account newaccount with vid vida
warning: transaction executed locally, but may not be confirmed by the network yet
```

下面假设测试网RPC URL 为 http://47.111.173.249:8001, 钱包URL 为 http://127.0.0.1:3333

可以使用

```bash
nocli --url http://47.111.173.249:8001 get info
```

来获取链信息.

## Create Account

账户名不超过12位，并且只能由这些字符组成：.12345abcdefghijklmnopqrstuvwxyz,

以 `bacc.`, `sys.` 或者 `rank.` 开头的账户为保留账户, 普通账户无法创建, 只有noc账户才可以创建.

VID格式与账户名相同.

> 注意：提交了创建用户名的交易，需等待到进入不可逆块。如果有其他人抢注这个账号，这个账号可能不属于你，如果刚创建就马上就给这个账号转账的话，可能钱就打水漂啦。

可以使用 get account 获取用户信息:

```bash
nocli --url http://47.111.173.249:8001 get account bacc.test
created: 2019-05-04T12:00:00.000
permissions:
     owner     1:    1 NOC5muUziYrETi5b6G2Ev91dCBrEm3qir7PK4S2qSFqfqcmouyzCr
        active     1:    1 NOC5muUziYrETi5b6G2Ev91dCBrEm3qir7PK4S2qSFqfqcmouyzCr
memory:
     quota:        64 KiB    used:      2.66 KiB

net bandwidth:
     used:                 0 bytes
     available:           32 KiB
     limit:               32 KiB

cpu bandwidth:
     used:                 0 us
     available:          800 ms
     limit:              800 ms
```

参数：

- creator TEXT  执行创建的账号 (必要)
- name TEXT     创建的新账号名 (必要)
- owner TEXT    拥有者的账号公钥 (必要)
- active TEXT   新账号的激活公钥，默认同拥有者的账号公钥 (必要)
- vid TEXT      该账户的vid (可选)

命令:

这里使用 `bacc.test` 账户创建 `nn` 账户, vid为`vid11111111`

```bash
nocli --wallet-url http://127.0.0.1:3333 --url http://47.111.173.249:8001 create account bacc.test nn NOC6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV NOC6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV "vid11111111"
```

或者:

```bash
nocli --wallet-url http://127.0.0.1:3333 --url http://47.111.173.249:8001 push action codex newaccount '{"creator":"bacc.test","name":"nn","owner":{"threshold": 1,"keys": [{"key": "NOC6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","weight": 1}],"accounts": [],"waits":[]},"active":{"threshold": 1,"keys": [{"key": "NOC6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","weight": 1}],"accounts": [],"waits":[]},"vid":"vid11111111"}' -p bacc.test
```

如果不想填vid则使用空字符串:

```bash
nocli --wallet-url http://127.0.0.1:3333 --url http://47.111.173.249:8001 push action codex newaccount '{"creator":"bacc.test","name":"nn","owner":{"threshold": 1,"keys": [{"key": "NOC6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","weight": 1}],"accounts": [],"waits":[]},"active":{"threshold": 1,"keys": [{"key": "NOC6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV","weight": 1}],"accounts": [],"waits":[]},"vid":""}' -p bacc.test
```

## Transfer Token

NOC的核心通证为NOC, 最小精度为 0.0001

代币转账操作

```cpp
 void transfer( account_name from,
                account_name to,
                asset        quantity,
                string       memo );
```

参数:

- from :转账的账户
- to : 接收币的账户
- quantity :转账的币
- memo : 备注

```bash
nocli --wallet-url http://127.0.0.1:3333 --url http://47.111.173.249:8001  transfer test.a test.b "10.0000 BACC" "mmm"
executed transaction: fdb7e7c7ab13177b0669ae307dfc31e79d162145709f52e88fca6bae1da68792  128 bytes  252 us
#     bacc.token <= bacc.token::transfer          {"from":"test.a","to":"test.b","quantity":"10.0000 BACC","memo":"mmm"}
#        test.a <= bacc.token::transfer          {"from":"test.a","to":"test.b","quantity":"10.0000 BACC","memo":"mmm"}
#        test.b <= bacc.token::transfer          {"from":"test.a","to":"test.b","quantity":"10.0000 BACC","memo":"mmm"}
warning: transaction executed locally, but may not be confirmed by the network yet
```

可以用 get current 获取用户通证:

```bash
nocli --wallet-url http://127.0.0.1:3333 --url http://47.111.173.249:8001 get currency balance bacc.token bacc.test BACC
10000000.0000 BACC
```

## Buy CPU, NET and RAM

可以通过 get account 获取用户详细信息:

```bash
nocli --wallet-url http://127.0.0.1:3333 --url http://47.111.173.249:8001 get account bacc.test
created: 2019-05-04T12:00:00.000
permissions:
     owner     1:    1 NOC5muUziYrETi5b6G2Ev91dCBrEm3qir7PK4S2qSFqfqcmouyzCr
        active     1:    1 NOC5muUziYrETi5b6G2Ev91dCBrEm3qir7PK4S2qSFqfqcmouyzCr
memory:
     quota:        64 KiB    used:      2.66 KiB

net bandwidth:
     used:                 0 bytes
     available:           32 KiB
     limit:               32 KiB

cpu bandwidth:
     used:                 0 us
     available:          800 ms
     limit:              800 ms
```

Codex的cpu和net通过抵押的方式获取:

```bash
nocli --wallet-url http://127.0.0.1:3333 --url http://47.111.173.249:8001 push action bacc delegatebw '["test.c","test.c","10.0000 BACC","10.0000 BACC",0]' -p test.c

nocli --wallet-url http://127.0.0.1:3333 --url http://47.111.173.249:8001 push action bacc delegatebw \
'{"from":"test.c","receiver":"test.c","stake_net_quantity":"10.0000 BACC","stake_cpu_quantity":"10.0000 BACC","transfer":0}' \
-p test.c
```

参数：

- from : 付款的账户
- receiver:接受CPU和NET的账户
- stake_net_quantity : 增加的NET的抵押
- stake_cpu_quantity : 增加的CPU的抵押
- transfer : 是否会把币转个receiver

内存待定, 当前状态下由于存在低保系统, 用户无需内存.


## Start Node

节点使用bin/noc开启,

配置:

```
http-validate-host=false
chain-state-db-size-mb = 8192
reversible-blocks-db-size-mb = 340
https-client-validate-peers = false
access-control-allow-origin = *
access-control-allow-headers = Content-Type
access-control-allow-credentials = false
p2p-max-nodes-per-host = 10
allowed-connection = any
max-clients = 50
connection-cleanup-period = 30
network-version-match = 0
sync-fetch-span = 100
max-implicit-request = 1500
enable-stale-production = false
pause-on-startup = false
max-transaction-time = 1000
max-irreversible-block-age = 1800
keosd-provider-timeout = 5

p2p-peer-address = 127.0.0.1:8101

plugin = eosio::chain_api_plugin
plugin = eosio::history_plugin
plugin = eosio::http_plugin
```

配置中主要需要修改`p2p-peer-address`为测试网的P2P Address, 目前为 47.111.173.249:9001

noc_node路径下有配置好的config目录, 其中需要包括核心合约文件.

使用noc可以开启测试节点:

```bash
./bin_osx/bacc --config-dir ./noc_node/config -d ./noc_node/datas
```

--config-dir 制定配置文件路径, -d 制定数据路径
