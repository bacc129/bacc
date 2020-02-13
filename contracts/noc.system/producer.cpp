#include "noc.system.hpp"
#include <cmath>
#include <set>
#include <noc.token/noc.token.hpp>

namespace noc {

asset calc_reward_by_block( const uint32_t block_num ) {
    const auto attenuation_cycle = block_num / reward_attenuation_cycle;

    // for 0.9 per attenuation if > 100, can not get reward
    if ( attenuation_cycle > 100 ) {
        return asset{};
    }

    const auto attenuation_coefficient = std::pow( 0.50, static_cast<double>( attenuation_cycle ) );
    if ( attenuation_coefficient < 0.0001 ) {
        return asset{};
    }

    return asset{ static_cast<int64_t>( reward_per_block * attenuation_coefficient * 10000 ) };
}

void system_contract::onblock( const block_timestamp,
                               const account_name bpname,
                               const uint16_t,
                               const block_id_type,
                               const checksum256,
                               const checksum256,
                               const uint32_t schedule_version ) {
    schedules_table schs_tbl( _self, _self );

    const auto current_block = current_block_num();

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

    auto reward_current_block = calc_reward_by_block( current_block );

    // to reward for vid
    auto reward_to_vid = asset{};
    const auto itr = _vid_params.find( vid_params_name );
    if ( itr != _vid_params.end() && current_block > 1 ) {
        const auto vs_itr = _vid_stats.find( static_cast<uint64_t>( current_block - 1 ) );
        if ( vs_itr != _vid_stats.end() ) {
            reward_to_vid = itr->reward_per_vid * itr->vid_reward_coefficient * vs_itr->vid_count;
        }
    }

    if( reward_to_vid > asset{} ){
        if( reward_current_block < reward_to_vid ){
            reward_current_block = asset{};
            reward_to_vid = reward_current_block;
        } else {
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

    auto reward_for_pow = asset{ 0, CORE_SYMBOL };
    if( reward_for_pow > reward_current_block ){
        reward_for_pow = reward_current_block;
    }
    if( reward_for_pow > asset{} ){
        eosio::token::issue_token_inline(
            ::config::pow_pool_account_name,
            reward_for_pow,
            "issue tokens for pow pool" );
    }

    reward_current_block -= reward_for_pow;
    const auto reward_for_produce = (reward_current_block * reward_for_produce_p) / 100;
    const auto reward_for_bps = (reward_current_block * reward_for_bps_p) / 100;
    const auto reward_for_user = reward_current_block - (reward_for_bps + reward_for_produce);

    //print( "reward:", current_block, ", ", reward_for_bps, ", ", reward_for_user, ", ", reward_to_vid, "\n" );

    // reward bps
    reward_bps_and_user( block_producers, bpname, reward_for_produce, reward_for_bps, reward_for_user );

    if ( current_block % UPDATE_CYCLE == 0 ) {
        // update schedule
        update_elected_bps( current_block );
    }
}

void system_contract::updatebp( const account_name bpname,
                                const account_name invite,
                                const public_key   block_signing_key,
                                const std::string& url ) {
    require_auth( bpname );
    eosio_assert( url.size() < 64, "url too long" );

    bpopened_table bpo( _self, _self );
    eosio_assert( bpo.find( bpname ) != bpo.end(), "bp account not allow" );

    auto      bp = _bps.find( bpname );
    if ( bp == _bps.end() ) {
        _bps.emplace( bpname, [&]( bp_info& b ) {
            b.name = bpname;
            b.invite = invite;
            b.update( block_signing_key, url );
        } );
    } else {
        _bps.modify( bp, 0, [&]( bp_info& b ) { b.update( block_signing_key, url ); } );
    }
}

void system_contract::removebp( account_name bpname ) {
    require_auth( _self );

    auto      bp = _bps.find( bpname );
    if ( bp == _bps.end() ) {
        eosio_assert( false, "bpname is not registered" );
    } else {
        _bps.modify( bp, 0, [&]( bp_info& b ) { b.deactivate(); } );
    }

    // TODO Return token when remove bp
}

void system_contract::update_elected_bps( const uint32_t curr_block_num ) {
    constexpr auto bps_top_size = static_cast<size_t>( NUM_OF_TOP_BPS );

    constexpr auto genesis_boot_end_block_num = 360 * BLOCK_NUM_PER_DAY;
    constexpr auto genesis_boot_bp_num        = 14;

    const auto is_in_genesis_boot = curr_block_num <= genesis_boot_end_block_num;

    std::vector<std::pair<eosio::producer_key, int64_t>> vote_schedule;
    vote_schedule.reserve( 32 );

    std::vector<std::pair<eosio::producer_key, int64_t>> vote_schedule_genesis;
    vote_schedule_genesis.reserve( 32 );

    for ( const auto& it : _bps ) {
        if ( is_in_genesis_boot && is_genesis_init_bp( it.name ) ) {
            vote_schedule_genesis.push_back( 
                std::make_pair( eosio::producer_key{ it.name, it.block_signing_key }, it.total_staked ) );
            continue;
        }

        const auto vs_size = vote_schedule.size();
        if ( vs_size >= bps_top_size && vote_schedule[vs_size - 1].second > it.total_staked ) {
            continue;
        }

        for ( int i = 0; i < bps_top_size; ++i ) {
            if ( vote_schedule[i].second <= it.total_staked ) {
                vote_schedule.insert(
                  vote_schedule.begin() + i,
                  std::make_pair( eosio::producer_key{ it.name, it.block_signing_key }, it.total_staked ) );

                if ( vote_schedule.size() > bps_top_size ) {
                    vote_schedule.resize( bps_top_size );
                }
                break;
            }
        }
    }

    if ( is_in_genesis_boot ) {
        auto bp_no_genesis_num = bps_top_size - genesis_boot_bp_num;
        bp_no_genesis_num = bp_no_genesis_num > vote_schedule.size() 
                            ? vote_schedule.size() : bp_no_genesis_num;

        for ( auto idx = 0; idx < bp_no_genesis_num; ++idx ) {
            vote_schedule_genesis.push_back(vote_schedule[idx]);
        }

        std::sort( vote_schedule_genesis.begin(), vote_schedule_genesis.end(), []( const auto& l, const auto& r ) -> bool {
            return l.second > r.second;
        } );

        vote_schedule_genesis.resize( bps_top_size );

    } else { 
        if ( vote_schedule.size() > bps_top_size ) {
            vote_schedule.resize( bps_top_size );
        }
    }

    std::vector<eosio::producer_key> vote_schedule_data;
    vote_schedule_data.reserve( vote_schedule.size() );

    if( is_in_genesis_boot ) {
        for ( const auto& v : vote_schedule_genesis ) {
            vote_schedule_data.push_back( v.first );
        }
    } else {
        for ( const auto& v : vote_schedule ) {
            vote_schedule_data.push_back( v.first );
        }
    }

    std::sort( vote_schedule_data.begin(), vote_schedule_data.end(), []( const auto& l, const auto& r ) -> bool {
        return l.producer_name < r.producer_name;
    } );

    auto packed_schedule = pack( vote_schedule_data );
    set_proposed_producers( packed_schedule.data(), packed_schedule.size() );
}

// TODO it need change if no bonus to accounts

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

    //calculate total staked all of the bps
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

} // namespace noc
