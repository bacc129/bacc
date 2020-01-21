# Token合约

token合约在noc.token, 在bacc中 默认部署在 bacc.token 内.

token合约和eos token合约相似, 可以参考:

- https://zhuanlan.zhihu.com/p/37300986
- https://segmentfault.com/a/1190000017184246
- https://cloud.tencent.com/developer/article/1182639

## 1. 基于bacc.token发行代币

与eosio不同, bacc可以直接基于系统的bacc.token合约发币, 需要 bacc.token的权限

```bash
./baccli -u http://47.98.238.9:15001 push action bacc.token create '{"issuer":"bacc","maximum_supply":"100000000000.0000 XXX"}' -p bacc.token
```

其他可以参考上面的文档, 区别就是合约不需要自身部署, 只需使用系统合约就行, 注意合约名.

内存问题, 所有以bacc.开头的账户不限制内存使用, 只能使用bacc.config账户创建, 如果是普通账户需要增加其内存上限:

```bash
./baccli -u http://47.98.238.9:15001 push action bacc setram '{"account":"test.c","stake":"100000.0000 BACC"}' -p bacc
```

这里的stake只是标示数量, 并不需要真正的token

## 2. 核心token的增发

每个区块会增发核心token bacc, 其分配逻辑在系统合约的onblock函数中, 在 noc.system中的producer.cpp文件中:

块奖励构成:

- 每个块的奖励 = 自留分配 + 挖矿奖励（投票挖矿（pos）+矿机挖矿（pow）（初期0））
- 挖矿奖励 = 块奖励 — 自留分配
- 投票奖励分配比例 = 10%出块奖励+10%节点奖励+80%用户投票奖励

修改合约之后只需重新部署系统合约即可, 使用bacc账户权限即可更新系统合约改变分发逻辑.

**注意** 部署之前必须测试完成, 一旦系统合约部署失败, 链将会故障, 很难修复!!!!!!!!!!!!!!!!!!!!!!!!!
**注意** 部署之前必须测试完成, 一旦系统合约部署失败, 链将会故障, 很难修复!!!!!!!!!!!!!!!!!!!!!!!!!
**注意** 部署之前必须测试完成, 一旦系统合约部署失败, 链将会故障, 很难修复!!!!!!!!!!!!!!!!!!!!!!!!!

onblock解析:

```cpp
void system_contract::onblock( const block_timestamp,
                               const account_name bpname,
                               const uint16_t,
                               const block_id_type,
                               const checksum256,
                               const checksum256,
                               const uint32_t schedule_version ) {
    schedules_table schs_tbl( _self, _self );

    // 获取当前区块高度
    const auto current_block = current_block_num();

    // 更新BP 列表
    account_name block_producers[NUM_OF_TOP_BPS] = {};
    get_active_producers( block_producers, sizeof( account_name ) * NUM_OF_TOP_BPS );
    auto sch = schs_tbl.find( uint64_t( schedule_version ) );
    if ( sch == schs_tbl.end() ) {
        schs_tbl.emplace( bpname, [&]( schedule_info& s ) {
            s.version      = schedule_version;
            s.block_height = current_block;
            for ( int i = 0; i < NUM_OF_TOP_BPS; i++ ) {
                s.producers[i].amount = block_producers[i] == bpname ? 1 : 0;
                s.producers[i].bpname = block_producers[i];
            }
        } );
    } else {
        schs_tbl.modify( sch, 0, [&]( schedule_info& s ) {
            for ( int i = 0; i < NUM_OF_TOP_BPS; i++ ) {
                if ( s.producers[i].bpname == bpname ) {
                    s.producers[i].amount += 1;
                    break;
                }
            }
        } );
    }

    // 这里开始分发奖励Token, 先计算应该分发的Token, 下面会解析计算方法
    auto reward_current_block = calc_reward_by_block( current_block );

    // 挖矿奖励
    // 挖矿奖励 =此周期（一个块或者）内vid的消耗币量（一个指定action的地址收了指定数量的系统代币）*调节系数（默认1，此调节系数可以通过超级权限调整）
    // vid_reward_coefficient 调节系数（默认1，此调节系数可以通过超级权限调整）
    auto reward_to_vid = asset{}; // 挖矿奖励
    const auto itr = _vid_params.find( vid_params_name ); // 此周期（一个块或者）内vid的消耗币量
    if ( itr != _vid_params.end() && current_block > 1 ) {
        const auto vs_itr = _vid_stats.find( static_cast<uint64_t>( current_block - 1 ) );
        if ( vs_itr != _vid_stats.end() ) {
            // 挖矿奖励 =此周期（一个块或者）内vid的消耗币量（一个指定action的地址收了指定数量的系统代币）*调节系数（默认1，此调节系数可以通过超级权限调整）
            reward_to_vid = itr->reward_per_vid * itr->vid_reward_coefficient * vs_itr->vid_count;
        }
    }

    if( reward_to_vid > asset{} ){
        if( reward_current_block < reward_to_vid ){
            reward_current_block = asset{};
            reward_to_vid = reward_current_block;
        } else {
            // 分配 = 每个块奖励-挖矿奖励
            reward_current_block -= reward_to_vid;
        }
        eosio::token::issue_token_inline(
            ::config::vid_pool_account_name,
            reward_to_vid,
            "issue tokens for vid pool" );
    }

    if( reward_current_block > asset{} ){
        eosio::token::issue_token_inline(
            ::config::system_account_name, 
            reward_current_block, 
            "issue tokens for block reward" );
    }

    // 矿机挖矿 pow 现在没有
    const auto reward_tobe_send = asset{ 1, CORE_SYMBOL };
    reward_current_block -= reward_tobe_send;

    // 投票奖励分配比例 = 10%出块奖励+10%节点奖励+80%用户投票奖励
    const auto reward_for_produce = (reward_current_block * 10) / 100; // 10%出块奖励
    const auto reward_for_bps = (reward_current_block * 10) / 100; // 10%节点奖励
    const auto reward_for_user = reward_current_block - (reward_for_bps + reward_for_produce); // 80%用户投票奖励

    //print( "reward:", current_block, ", ", reward_for_bps, ", ", reward_for_user, ", ", reward_to_vid, "\n" );

    // 分发投票奖励
    reward_bps_and_user( block_producers, bpname, reward_for_produce, reward_for_bps, reward_for_user );

    // 更新BP排行
    if ( current_block % UPDATE_CYCLE == 0 ) {
        // update schedule
        update_elected_bps( current_block );
    }
}
```

先看看计算奖励总量函数:

```cpp
// 奖励会随着时间减半
asset calc_reward_by_block( const uint32_t block_num ) {
    const auto attenuation_cycle = block_num / reward_attenuation_cycle;

    // for 0.9 per attenuation if > 100, can not get reward
    if ( attenuation_cycle > 100 ) {
        return asset{};
    }

    const auto attenuation_coefficient = std::pow( 0.50, static_cast<double>( attenuation_cycle ) );
    if ( attenuation_coefficient < 0.0001 ) { // 如果很少就不发了
        return asset{};
    }

    return asset{ static_cast<int64_t>( reward_per_block * attenuation_coefficient * 10000 ) };
}
```

具体的分发:

```cpp
// 投票奖励分配比例 = 10%出块奖励+10%节点奖励+80%用户投票奖励
// 这里基本没有改的必要
void system_contract::reward_bps_and_user( account_name block_producers[], 
                                           account_name producer,
                                           const asset& for_produce,
                                           const asset& for_bp, 
                                           const asset& for_user ) {
    if ( (for_bp.amount == 0) && (for_user.amount == 0) ) {
        return;
    }

    std::set<account_name> block_producers_set;
    for ( auto i = 0; i < NUM_OF_TOP_BPS; i++ ) {
        block_producers_set.insert(block_producers[i]);
    }

    // 按照投票给bp分发, 用户奖励分给
    int64_t staked_all_prodeucers      = 0;
    int64_t staked_all_block_producers = 0;
    for( auto it = _bps.cbegin(); it != _bps.cend(); ++it ) {
         staked_all_prodeucers += it->total_staked;
         if( block_producers_set.find(it->name) != block_producers_set.end() ) {
             staked_all_block_producers += it->total_staked;
         }
    }

    if( staked_all_prodeucers <= 0 || staked_all_prodeucers <= 0 ) {
        return;
    }

    for ( auto it = _bps.cbegin(); it != _bps.cend(); ++it ) {
        if ( !it->isactive ) {
            continue;
        }

        const auto reward_pool_amount = static_cast<int64_t>( 
            for_user.amount * double(it->total_staked) / double(staked_all_prodeucers));

        int64_t reward_block_amount = 0;
        if( block_producers_set.find(it->name) != block_producers_set.end() ) {
            reward_block_amount = static_cast<int64_t>( 
                for_bp.amount * double(it->total_staked) / double(staked_all_block_producers));
        }

        if( producer == it->name ) {
            reward_block_amount += for_produce.amount;
        }

        const auto& bp = _bps.get( it->name, "bpname is not registered" );
        _bps.modify( bp, 0, [&]( bp_info& b ) {
            b.rewards_pool += asset( reward_pool_amount );
            b.rewards_block += asset( reward_block_amount );
        } );
    }
}
```

## 更新系统合约

编译项目, 合约编译出来在 build/contracts/noc.system/ 下, 主要需要 noc.system.abi 和 noc.system.wasm 两个文件

```bash
./baccli -u http://47.98.238.9:15001 set contract bacc path/to/build/contracts/noc.system
```

注意, 如果合约二进制没有变化, 则不会更新, 更新需要系统合约权限

**注意** 部署之前必须测试完成, 一旦系统合约部署失败, 链将会故障, 很难修复!!!!!!!!!!!!!!!!!!!!!!!!!
**注意** 部署之前必须测试完成, 一旦系统合约部署失败, 链将会故障, 很难修复!!!!!!!!!!!!!!!!!!!!!!!!!
**注意** 部署之前必须测试完成, 一旦系统合约部署失败, 链将会故障, 很难修复!!!!!!!!!!!!!!!!!!!!!!!!!
