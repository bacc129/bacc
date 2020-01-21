# 利用超级权限更新

## 1. 编译

需要编译项目，BACC的合约和链在一个项目中，一次编译会编译出所有文件。

编译结果这样的：

```bash
├── build
    ├── bin
    │   ├── bacc        --> bacc节点进程
    │   ├── baccgenesis --> 生成genesis工具
    │   ├── baccli      --> bacc客户端
    │   └── baccwallet  --> bacc钱包服务
    └── contracts
        ├── noc.hello
        │   ├── noc.hello.abi  --> 一个测试实例合约abi
        │   └── noc.hello.wasm --> 一个测试实例合约wasm
        ├── noc.msig
        │   ├── noc.msig.abi   --> 多签合约
        │   └── noc.msig.wasm  --> 多签合约
        ├── noc.system
        │   ├── noc.system.abi  --> 系统合约
        │   └── noc.system.wasm --> 系统合约
        └── noc.token
            ├── noc.token.abi  --> token合约
            └── noc.token.wasm --> token合约
```

注意：

- 编译需要在ubuntu 18.04系统下，其他ubuntu或linux系统编译会出错。
- 最好不要在编译过eosio的机器上进行编译，如果编译过eosio就删去其在公共位置上建立的依赖库
- 编译需要在git 签出的目录下，否则会出错

首先需要准备好一个ubuntu 18.04环境，将其sshkey添加到github对应账号，签出github上的项目：

```bash
git clone git@github.com:bacc129/bacc.git
```

成功：

```bash
git clone git@github.com:bacc129/bacc.git
Cloning into 'bacc'...
Connection to github.com 22 port [tcp/ssh] succeeded!
remote: Enumerating objects: 5458, done.
remote: Counting objects: 100% (5458/5458), done.
remote: Compressing objects: 100% (3238/3238), done.
remote: Total 5458 (delta 2060), reused 5458 (delta 2060), pack-reused 0
Receiving objects: 100% (5458/5458), 7.76 MiB | 1.26 MiB/s, done.
Resolving deltas: 100% (2060/2060), done.
```

之后利用脚本进行编译：

```bash
cd bacc
./build.sh
```

首次编译需要安装依赖库，需要确认

成功：

```bash
Scanning dependencies of target bacc
[100%] Building CXX object programs/nodeos/CMakeFiles/bacc.dir/main.cpp.o
[100%] Linking CXX executable baccli
[100%] Built target baccli
[100%] Linking CXX executable bacc
[100%] Built target bacc
```

编译结果这样的：

```bash
├── build
    ├── bin
    │   ├── bacc        --> bacc节点进程
    │   ├── baccgenesis --> 生成genesis工具
    │   ├── baccli      --> bacc客户端
    │   └── baccwallet  --> bacc钱包服务
    └── contracts
        ├── noc.hello
        │   ├── noc.hello.abi  --> 一个测试实例合约abi
        │   └── noc.hello.wasm --> 一个测试实例合约wasm
        ├── noc.msig
        │   ├── noc.msig.abi   --> 多签合约
        │   └── noc.msig.wasm  --> 多签合约
        ├── noc.system
        │   ├── noc.system.abi  --> 系统合约
        │   └── noc.system.wasm --> 系统合约
        └── noc.token
            ├── noc.token.abi  --> token合约
            └── noc.token.wasm --> token合约
```

## 2. 修改系统合约以调整激励参数

如果不熟悉合约开发，不建议对主要逻辑进行修改，只改动参数即可。

需要修改 /contracts/noc.system/configs.hpp :

```cpp
#pragma once

// 出块机制: 0.5秒一个块，一个节点连续出6个块
// 每个块发出 0.5个币，每34560000个块（约200天），块奖励减少50%

static constexpr double   reward_per_block         = 0.5;       // 每个块发出 0.5个币
static constexpr uint32_t reward_attenuation_cycle = 34560000U; // 每34560000个块（约200天），块奖励减少50%，注意这个修改的话会立即影响，不建议修改，修改前需要仔细阅读代码

// 块奖励构成
//   每个块的奖励 =自留分配+挖矿奖励（投票挖矿（pos）+矿机挖矿（pow）（初期0））
//   挖矿奖励 = 块奖励 — 自留分配
//   投票奖励分配比例 = 10% 出块奖励 + 10% 节点奖励 + 80% 用户投票奖励
static constexpr int64_t reward_for_produce_p  = 10; // 10% 出块奖励
static constexpr int64_t reward_for_bps_p      = 10; // 10% 节点奖励
// reward_for_user 剩下的都是用户投票奖励， 用户投票奖励 是 100 减去上面两个值
```

这个文件中可以修改参数，比如如果改成每个块产出3个Token，则可以改为

```cpp
#pragma once

// 出块机制: 0.5秒一个块，一个节点连续出6个块
// 每个块发出 0.5个币，每34560000个块（约200天），块奖励减少50%

static constexpr double   reward_per_block         = 3;       // 每个块发出 （0.5个币） --> 3
static constexpr uint32_t reward_attenuation_cycle = 34560000U; // 每34560000个块（约200天），块奖励减少50%，注意这个修改的话会立即影响，不建议修改，修改前需要仔细阅读代码

// 块奖励构成
//   每个块的奖励 =自留分配+挖矿奖励（投票挖矿（pos）+矿机挖矿（pow）（初期0））
//   挖矿奖励 = 块奖励 — 自留分配
//   投票奖励分配比例 = 10% 出块奖励 + 10% 节点奖励 + 80% 用户投票奖励
static constexpr int64_t reward_for_produce_p  = 10; // 10% 出块奖励
static constexpr int64_t reward_for_bps_p      = 10; // 10% 节点奖励
// reward_for_user 剩下的都是用户投票奖励， 用户投票奖励 是 100 减去上面两个值
```

任何改动之后需要重新编译， 按照上面的介绍编译。

## 3. 更新链上合约

编译项目, 合约编译出来在 build/contracts/noc.system/ 下, 主要需要 noc.system.abi 和 noc.system.wasm 两个文件

```bash
./baccli -u http://47.98.238.9:15001 set contract bacc path/to/build/contracts/noc.system
```

注意, 如果合约二进制没有变化, 则不会更新, 更新需要超级权限，即bacc账户对应的权限

**注意** 部署之前必须测试完成, 一旦系统合约部署失败, 链将会故障, 很难修复!!!!!!!!!!!!!!!!!!!!!!!!!
**注意** 部署之前必须测试完成, 一旦系统合约部署失败, 链将会故障, 很难修复!!!!!!!!!!!!!!!!!!!!!!!!!
**注意** 部署之前必须测试完成, 一旦系统合约部署失败, 链将会故障, 很难修复!!!!!!!!!!!!!!!!!!!!!!!!!
