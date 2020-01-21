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