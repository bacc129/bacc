#include "noc.system.hpp"
#include <noc.token/noc.token.hpp>

#include "native.cpp"
#include "producer.cpp"
#include "vote4ram.cpp"
#include "voting.cpp"

#if CONTRACT_RESOURCE_MODEL == RESOURCE_MODEL_DELEGATE
#include "delegate_bandwidth.cpp"
#endif

namespace noc {
void system_contract::setparams( const eosio::blockchain_parameters& params ) {
    require_auth( _self );
    eosio_assert( 3 <= params.max_authority_depth, "max_authority_depth should be at least 3" );
    set_blockchain_parameters( params );
}

void system_contract::newaccount( account_name, account_name new_name, authority, authority, account_name vid ) {
#if CONTRACT_RESOURCE_MODEL == RESOURCE_MODEL_DELEGATE

    user_resources_table userres( _self, new_name );

    userres.emplace( new_name, [&]( auto& res ) { res.owner = new_name; } );

    set_resource_limits( new_name, 0, 0, 0 );
#endif

    const auto vid_name = eosio::name{ vid };

    print( "new account ", eosio::name{ new_name }, " with vid ", vid_name );
    eosio_assert( _vids.find( vid ) == _vids.end(), "vid has been used" );
    _vids.emplace( new_name, [&]( auto& b ) {
        b.vid     = vid_name;
        b.account = new_name;
    } );

    const auto curr_block = current_block_num();

    auto itr = _vid_stats.find( static_cast<uint64_t>( curr_block ) );

    if ( itr == _vid_stats.end() ) {
        _vid_stats.emplace( _self, [&]( vid_state& vs ) { 
            vs.block_num = curr_block;
            vs.vid_count = 1;
            vs.vids.emplace_back(vid_info{ vid_name, new_name });
        } );
    } else {
        _vid_stats.modify( itr, _self, [&]( vid_state& vs ) {
            vs.vid_count += 1;
            vs.vids.emplace_back(vid_info{ vid_name, new_name });
        } );
    }
}

void system_contract::openbp( account_name producer ) {
    require_auth( _self );

    bpopened_table bpo( _self, _self );
    eosio_assert( bpo.find( producer ) == bpo.end(), "producer has into opened list" );
    bpo.emplace( _self, [&]( auto& bp ) { bp.bpname = producer; } );
}

void system_contract::banbp( account_name producer ) {
    require_auth( _self );

    eosio_assert(!is_genesis_init_bp(producer), "cannot ban genesis bp");

    bpopened_table bpo( _self, _self );
    auto           itr = bpo.find( producer );
    eosio_assert( itr != bpo.end(), "producer not in opened list" );
    bpo.erase( itr );
}

void system_contract::bppawn( account_name bpname, asset pawn ) {
    require_auth( bpname );

    eosio_assert( !is_genesis_init_bp(bpname), "genesis bp cannot bppawn");
    eosio_assert( pawn > asset{}, "pawn should > 0" );

    auto bp = _bps.find( bpname );
    eosio_assert( bp != _bps.end(), "block producer not reg" );
    _bps.modify( bp, 0, [&]( bp_info& b ) { 
        b.pawn += pawn;
    } );

    INLINE_ACTION_SENDER( eosio::token, transfer )
        ( config::token_account_name,
        { bpname, N( active ) },
        { bpname, config::bp_pawn_account_name, pawn, "pawn for bp" } );
}

void system_contract::bpredeem( account_name bpname ) {
    require_auth( bpname );

    auto bp = _bps.find( bpname );

    eosio_assert( !is_genesis_init_bp(bpname), "genesis bp cannot bpredeem");
    eosio_assert( bp != _bps.end(), "block producer not reg" );
    eosio_assert( bp->total_staked <= 0, "bp total staked should be zero" );

    const auto all_pawn = bp->pawn;

    INLINE_ACTION_SENDER( eosio::token, transfer )
        ( config::token_account_name,
        { config::bp_pawn_account_name, N( active ) },
        { config::bp_pawn_account_name, bpname, all_pawn, "redeem pawn for bp" } );

    _bps.modify( bp, 0, [&]( bp_info& b ) { 
        b.pawn = asset{};
    } );
}

void system_contract::bpclaim( account_name bpname ) {
    require_auth( bpname );

    auto bp = _bps.find( bpname );

    eosio_assert( bp != _bps.end(), "block producer not reg" );
    eosio_assert( bp->rewards_block >= asset{ 500 * 10000 }, "bp reward max >= 500.0000 BACC" );

    const auto fee = (bp->rewards_block + asset{ 50 }) / 100;
    const auto reward = bp->rewards_block - fee;

    eosio::token::send_token_inline( config::system_account_name, bpname, reward, "bp claim block reward" );
    eosio::token::send_token_inline( config::system_account_name, config::ecosystem_account_name, 
                                     fee, "bp claim block reward fee" );

    _bps.modify( bp, 0, [&]( bp_info& b ) { 
        b.rewards_block = asset{};
    } );
}

void system_contract::setvidparam( uint64_t coefficient, asset reward_per_vid ) {
    require_auth( _self );

    auto itr = _vid_params.find( vid_params_name );

    if ( itr == _vid_params.end() ) {
        _vid_params.emplace( _self, [&]( auto& vs ) { 
            vs.id = vid_params_name;
            vs.vid_reward_coefficient = coefficient;
            vs.reward_per_vid = reward_per_vid;
        } );
    } else {
        _vid_params.modify( itr, _self, [&]( auto& vs ) {
            vs.vid_reward_coefficient = coefficient;
            vs.reward_per_vid = reward_per_vid;
        } );
    }
}

} // namespace noc
