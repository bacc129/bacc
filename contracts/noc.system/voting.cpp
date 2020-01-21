#include <utility>

#include "noc.system.hpp"
#include "noc.token/noc.token.hpp"

namespace noc {

using namespace eosio;

void system_contract::freeze( const account_name voter, const asset stake ) {
    require_auth( voter );

    eosio_assert( stake.symbol == CORE_SYMBOL, "only support CORE SYMBOL token" );
    eosio_assert( 0 <= stake.amount && stake.amount % 10000 == 0, "need stake quantity >= 0 and quantity is integer" );

    auto tobe_staked = stake;
    auto vts = _voters.find( voter );
    if ( vts != _voters.end() ) {
        tobe_staked += vts->voteall;
    }
 
    const auto curr_block_num = current_block_num();

    auto         change = stake;
    auto         fts = _freezes.find( voter );
    if ( fts == _freezes.end() ) {
        _freezes.emplace( voter, [&]( freeze_info& v ) {
            v.voter          = voter;
            v.staked         = stake;
            v.unstake_height = curr_block_num;
        } );
    } else {
        change -= fts->staked;
        _freezes.modify( fts, 0, [&]( freeze_info& v ) {
            v.staked = stake;
            if ( change < asset{ 0 } ) {
                v.unstaking += ( -change );
                v.unstake_height = curr_block_num;
            }
        } );
        tobe_staked += fts->unstaking;
    }

    token::accounts account_tbl( config::token_account_name, voter );
    const auto& ac = account_tbl.get( stake.symbol.name(), "no find voter account!" );
    eosio_assert( ac.balance >= tobe_staked, "not have so much tokens!" );
}

// vote vote to a bp from voter to bpname with stake EOSC
void system_contract::vote( const account_name voter, const account_name bpname, const asset stake ) {
    require_auth( voter );

    const auto& bp = _bps.get( bpname, "bpname is not registered" );

    eosio_assert( !is_genesis_init_bp(bpname), "genesis bp cannot vote");

    eosio_assert( stake.symbol == CORE_SYMBOL, "only support CORE SYMBOL token" );
    eosio_assert( (100000 <= stake.amount && stake.amount % 100000 == 0) || (stake.amount == 0), 
        "need stake quantity >= 10.0000 BACC and vote stake by power is integer" );

    const auto curr_block_num = current_block_num();

    auto        change = stake;
    votes_table votes_tbl( _self, voter );
    auto        vts = votes_tbl.find( bpname );
    if ( vts == votes_tbl.end() ) {
        // First vote the bp, it will cause ram add
        votes_tbl.emplace( voter, [&]( vote_info& v ) {
            v.bpname                = bpname;
            v.vote                  = stake;
            v.votepower             = ( stake / 10 );
            v.voteage_update_height = curr_block_num;
        } );
    } else {
        change -= vts->vote;
        votes_tbl.modify( vts, 0, [&]( vote_info& v ) {
            v.voteage += ( v.vote.amount / 10000 ) * ( curr_block_num - v.voteage_update_height );
            v.voteage_update_height = curr_block_num;
            v.vote                  = stake;
            v.votepower             = ( stake / 10 );
            if ( change < asset{} ) {
                auto fts = _freezes.find( voter );
                if ( fts == _freezes.end() ) {
                    _freezes.emplace( voter, [&]( freeze_info& f ) {
                        f.voter          = voter;
                        f.staked         = ( -change );
                        f.unstake_height = curr_block_num;
                    } );
                } else {
                    _freezes.modify( fts, 0, [&]( freeze_info& f ) {
                        f.staked += ( -change );
                        f.unstake_height = curr_block_num;
                    } );
                }
            }
        } );
    }

    eosio_assert( bp.isactive || ( !bp.isactive && change < asset{ 0 } ), "bp is not active" );

    // Check 
    if ( change > asset{} ) {
        // check bp pawn token is enough
        // >5000 - 800 * 10000 limit
        // >10000 - 2000 * 10000 limit
        // >30000   no limit
        const auto current_pawn = bp.pawn;
        const auto new_total_staked = bp.total_staked + ( change.amount / (10 * 10000) );
        eosio_assert(  ( (current_pawn >= asset{ 5000 * 10000}) && (new_total_staked <=  800 * 10000 ) ) 
                    || ( (current_pawn >= asset{10000 * 10000}) && (new_total_staked <= 2000 * 10000 ) ) 
                    || ( (current_pawn >= asset{30000 * 10000}) ),
        "bp pawn no enough");

        auto fts = _freezes.find( voter );
        eosio_assert( fts != _freezes.end() && fts->staked >= change, "voter freeze token < vote token" );
        _freezes.modify( fts, 0, [&]( freeze_info& v ) { v.staked -= change; } );
    }

    update_voter_info( voter, bpname, change );

    modify_bp_vote( bp, ( change.amount / (10 * 10000) ), curr_block_num );
}

void system_contract::votestatic( const account_name voter,
                                  const account_name bpname,
                                  const asset        stake,
                                  const uint32_t     typ ) {
    require_auth( voter );

    eosio_assert( !is_genesis_init_bp(bpname), "genesis bp cannot votestatic");

    const static auto static_vote_data = vector<std::pair<uint32_t, uint64_t>>{ 
        { 30,   1 }, 
        { 90,   3 }, 
        { 180,  6 }, 
        { 270,  9 }, 
        { 360, 12 }, 
    };

    eosio_assert( typ < static_cast<uint32_t>( static_vote_data.size() ), "typ param error" );

    const auto curr_block_num = current_block_num();
    const auto lock_block_num = static_vote_data[typ].first * BLOCK_NUM_PER_DAY;
    const auto vote_power     = static_vote_data[typ].second;

    eosio_assert( stake.symbol == CORE_SYMBOL, "only support CORE SYMBOL token" );
    eosio_assert( 0 <= stake.amount && stake.amount % 10000 == 0, "need stake quantity >= 0 and quantity is integer" );

    const auto& bp = _bps.get( bpname, "bpname is not registered" );
    eosio_assert( bp.isactive, "bp is not active" );

    // Check
    // check bp pawn token is enough
    // >5000 - 800 * 10000 limit
    // >10000 - 2000 * 10000 limit
    // >30000   no limit
    const auto current_pawn     = bp.pawn;
    const auto new_total_staked = bp.total_staked + ( stake.amount / 10000 );
    eosio_assert( ( ( current_pawn >= asset{ 5000 * 10000 } ) && ( new_total_staked <= 200 * 10000 ) ) 
               || ( ( current_pawn >= asset{ 30000 * 10000 } ) && ( new_total_staked <= 1000 * 10000 ) ) 
               || ( ( current_pawn >= asset{ 50000 * 10000 } ) ),
                  "bp pawn no enough" );

    auto fts = _freezes.find( voter );
    eosio_assert( fts != _freezes.end() && fts->staked >= stake, "voter freeze token < vote token" );
    _freezes.modify( fts, 0, [&]( freeze_info& v ) { v.staked -= stake; } );

    // update vote data
    votes_table votes_tbl( _self, voter );
    auto        vts = votes_tbl.find( bpname );
    if ( vts == votes_tbl.end() ) {
        // First vote the bp, it will cause ram add
        votes_tbl.emplace( voter, [&]( vote_info& v ) {
            v.bpname = bpname;
            v.append_static_vote( stake, vote_power, lock_block_num, curr_block_num );
        } );
    } else {
        votes_tbl.modify( vts, 0, [&]( vote_info& v ) {
            v.append_static_vote( stake, vote_power, lock_block_num, curr_block_num );
        } );
    }

    update_voter_info( voter, bpname, stake );

    modify_bp_vote( bp, ( (stake * vote_power).amount / 10000 ), curr_block_num );
}

void system_contract::quitsvote( const account_name voter, const account_name bpname, const uint32_t idx ) {
    require_auth( voter );
    
    const auto curr_block_num = current_block_num();
    
    const auto& bp = _bps.get( bpname, "bpname is not registered" );
    
    votes_table votes_tbl( _self, voter );
    const auto vts = votes_tbl.find( bpname );
    eosio_assert( (vts != votes_tbl.end()) 
               && ( static_cast<uint32_t>(vts->static_votes.size()) > idx)
               && ( !vts->static_votes[idx].is_has_quited ), 
        "no static vote data found" );
    
    const auto& static_vote_data = vts->static_votes[idx];
    eosio_assert( static_vote_data.unfreeze_block_num <= curr_block_num, 
        "static vote not unfreeze block num" );
        
    // quit
    const auto& vote_token = static_vote_data.vote;
         
    // add to freezes table
    auto fts = _freezes.find( voter );
    if ( fts == _freezes.end() ) {
        _freezes.emplace( voter, [&]( freeze_info& f ) {
            f.voter          = voter;
            f.staked         = vote_token;
            f.unstake_height = curr_block_num;
        } );
    } else {
        _freezes.modify( fts, 0, [&]( freeze_info& f ) {
            f.staked += vote_token;
            f.unstake_height = curr_block_num;
        } );
    }

    modify_bp_vote( bp, -( static_vote_data.votepower.amount / 10000 ), curr_block_num );
    
    votes_tbl.modify( vts, 0, [&]( vote_info& v ) {
        auto& static_vote = v.static_votes[idx];
        static_vote.is_has_quited = true;
        static_vote.votepower_age += 
            ((static_vote.votepower * ( curr_block_num - static_vote.votepower_age_update_height )).amount / 10000);
        static_vote.votepower_age_update_height = curr_block_num;
        static_vote.votepower = asset{};
        static_vote.vote = asset{};
    } );

    update_voter_info( voter, bpname, -vote_token );
}

void system_contract::votebyreward( const account_name voter, const account_name bpname ) {
    require_auth( voter );

    const auto& bp = _bps.get( bpname, "bpname is not registered" );
    eosio_assert( !is_genesis_init_bp(bpname), "genesis bp cannot votebyreward");
    eosio_assert( bp.isactive, "bp is not active" );

    votes_table votes_tbl( _self, voter );
    const auto& vts = votes_tbl.get( bpname, "voter have not add votes to the the producer yet" );

    const auto curr_block_num    = current_block_num();
    const auto mini_reward_chaim = asset{ 50 * 10000 };

    const auto newest_total_voteage =
      static_cast<int128_t>( bp.total_voteage + bp.total_staked * ( curr_block_num - bp.voteage_update_height ) );
    eosio_assert( 0 < newest_total_voteage, "claim is not available yet" );

    int128_t newest_voteage = 0;

    // From normal
    newest_voteage += vts.voteage;
    newest_voteage += ((vts.votepower * ( curr_block_num - vts.voteage_update_height )).amount / 10000);

    // From Static vote
    for( const auto& static_vote : vts.static_votes ){
        newest_voteage += static_vote.votepower_age;
        if( !static_vote.is_has_quited ){
            newest_voteage += ((static_vote.votepower * ( curr_block_num - static_vote.votepower_age_update_height )).amount / 10000);
        }
    }

    //print("chaim ", newest_voteage, " ", newest_total_voteage, "\n");

    const auto amount_voteage = static_cast<int128_t>( bp.rewards_pool.amount ) * newest_voteage;
    const auto reward = asset( static_cast<int64_t>( amount_voteage / newest_total_voteage ) );
    eosio_assert( asset{} < reward && reward <= bp.rewards_pool, "need 0 <= claim reward quantity <= rewards_pool" );

    auto reward_all = asset{ (reward.amount - (reward.amount % 100000)) };
    const auto vote_power = reward_all / 10;

    //print("chaim ", reward_all.amount, " ", bp.rewards_pool.amount, "\n");

    eosio_assert( reward_all >= mini_reward_chaim, "reward should more then 100.0000 BACC!" );

    votes_tbl.modify( vts, 0, [&]( vote_info& v ) {
        v.voteage               = 0;
        v.voteage_update_height = curr_block_num;
        v.vote                  += reward_all;
        v.votepower             += vote_power;
        
        auto is_has_some_static_vote_quited = false;
        for( auto& sv : v.static_votes ){
            if( sv.is_has_quited ){
                is_has_some_static_vote_quited = true;
            }
            sv.votepower_age = 0;
            sv.votepower_age_update_height = curr_block_num;
        }
        
        if( is_has_some_static_vote_quited ){
            std::vector<vote_info_item> new_votes;
            new_votes.reserve(v.static_votes.size());
            for( const auto& sv : v.static_votes ){
                if( !sv.is_has_quited ){
                    new_votes.push_back(sv);
                }
            }
            v.static_votes = new_votes;
        }
    } );

    _bps.modify( bp, 0, [&]( bp_info& b ) {
        b.rewards_pool          -= reward;
        b.total_voteage         = static_cast<int64_t>( newest_total_voteage - newest_voteage );
        b.voteage_update_height = curr_block_num;
    } );

    update_voter_info( voter, bpname, reward_all );

    modify_bp_vote( bp, ( vote_power.amount / 10000 ), curr_block_num );
}

void system_contract::unfreeze( const account_name voter ) {
    require_auth( voter );

    const auto   itr = _freezes.get( voter, "voter have not freeze token yet" );

    const auto curr_block_num = current_block_num();

    eosio_assert( itr.unstake_height + FROZEN_DELAY < curr_block_num, "unfreeze is not available yet" );
    eosio_assert( 0 < itr.unstaking.amount, "need unstaking quantity > 0" );

    INLINE_ACTION_SENDER( eosio::token, transfer )
    ( config::token_account_name,
      { ::config::system_account_name, N( active ) },
      { ::config::system_account_name, voter, itr.unstaking, "unfreeze" } );
}

void system_contract::claim( const account_name voter, const account_name bpname ) {
    require_auth( voter );

    const auto& bp = _bps.get( bpname, "bpname is not registered" );

    votes_table votes_tbl( _self, voter );
    const auto& vts = votes_tbl.get( bpname, "voter have not add votes to the the producer yet" );

    const auto curr_block_num    = current_block_num();
    const auto fee_per_chaim     = asset{ 10 * 10000 };
    const auto mini_reward_chaim = asset{ 50 * 10000 };

    const auto newest_total_voteage =
      static_cast<int128_t>( bp.total_voteage + bp.total_staked * ( curr_block_num - bp.voteage_update_height ) );
    eosio_assert( 0 < newest_total_voteage, "claim is not available yet" );

    int128_t newest_voteage = 0;

    // From normal
    newest_voteage += vts.voteage;
    newest_voteage += ((vts.votepower * ( curr_block_num - vts.voteage_update_height )).amount / 10000);

    // From Static vote
    for( const auto& static_vote : vts.static_votes ){
        newest_voteage += static_vote.votepower_age;
        if( !static_vote.is_has_quited ){
            newest_voteage += ((static_vote.votepower * ( curr_block_num - static_vote.votepower_age_update_height )).amount / 10000);
        }
    }

    print("chaim ", newest_voteage, " ", newest_total_voteage, "\n");

    const auto amount_voteage = static_cast<int128_t>( bp.rewards_pool.amount ) * newest_voteage;
    const auto reward = asset( static_cast<int64_t>( amount_voteage / newest_total_voteage ) );
    eosio_assert( asset{} < reward && reward <= bp.rewards_pool, "need 0 <= claim reward quantity <= rewards_pool" );

    auto reward_all = reward;

    print("chaim ", reward_all.amount, " ", bp.rewards_pool.amount, "\n");

    eosio_assert( reward_all > mini_reward_chaim, "reward should more then 50.0000 BACC!" );
    reward_all -= fee_per_chaim;

    INLINE_ACTION_SENDER( eosio::token, transfer )
        ( config::token_account_name,
        { config::system_account_name, N( active ) },
        { config::system_account_name, voter, reward_all, "claim" } );

    INLINE_ACTION_SENDER( eosio::token, transfer )
        ( config::token_account_name,
        { config::system_account_name, N( active ) },
        { config::system_account_name, config::develop_account_name, fee_per_chaim, "claim fee" } );

    votes_tbl.modify( vts, 0, [&]( vote_info& v ) {
        v.voteage               = 0;
        v.voteage_update_height = curr_block_num;
        
        auto is_has_some_static_vote_quited = false;
        for( auto& sv : v.static_votes ){
            if( sv.is_has_quited ){
                is_has_some_static_vote_quited = true;
            }
            sv.votepower_age = 0;
            sv.votepower_age_update_height = curr_block_num;
        }
        
        if( is_has_some_static_vote_quited ){
            std::vector<vote_info_item> new_votes;
            new_votes.reserve(v.static_votes.size());
            for( const auto& sv : v.static_votes ){
                if( !sv.is_has_quited ){
                    new_votes.push_back(sv);
                }
            }
            v.static_votes = new_votes;
        }
        
    } );

    _bps.modify( bp, 0, [&]( bp_info& b ) {
        b.rewards_pool -= reward;
        b.total_voteage         = static_cast<int64_t>( newest_total_voteage - newest_voteage );
        b.voteage_update_height = curr_block_num;
    } );
}
} // namespace noc
