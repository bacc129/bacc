#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/chain.h>
#include <eosiolib/contract.hpp>
#include <eosiolib/contract_config.hpp>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/producer_schedule.hpp>
#include <eosiolib/time.hpp>
#include <bits/stdint.h>
#include <vector>

#include <noc.system/configs.hpp>

namespace noc {

using eosio::asset;
using eosio::block_timestamp;
using eosio::bytes;
using eosio::permission_level;
using eosio::print;
using std::string;
using std::vector;

static constexpr uint32_t FROZEN_DELAY   = CONTRACT_FROZEN_DELAY;
static constexpr int      NUM_OF_TOP_BPS = CONTRACT_NUM_OF_TOP_BPS; 
static constexpr uint32_t UPDATE_CYCLE   = CONTRACT_UPDATE_CYCLE;

static constexpr uint32_t BLOCK_NUM_PER_DAY        = 24 * 3600 * 2; // 0.5s a block, so a day will 172800 block

static constexpr uint64_t vid_params_name = N(vid);

struct permission_level_weight {
    permission_level permission;
    weight_type      weight;
};

struct key_weight {
    public_key  key;
    weight_type weight;
};

struct wait_weight {
    uint32_t    wait_sec;
    weight_type weight;
};

struct authority {
    uint32_t                        threshold = 0;
    vector<key_weight>              keys;
    vector<permission_level_weight> accounts;
    vector<wait_weight>             waits;
};

class system_contract : private eosio::contract {
  public:
    system_contract( account_name self )
      : contract( self )
      , _bps( _self, _self )
      , _freezes( _self, _self )
      , _vids( _self, _self )
      , _vid_stats( _self, _self )
      , _voters( _self, _self )
      , _vid_params( _self, _self ) {}

  private:

    // one type item vote info for a voter to a bp
    struct vote_info_item {
        asset        vote                        = asset{ 0 };
        asset        votepower                   = asset{ 0 };
        int64_t      votepower_age               = 0; // asset.amount * block height
        uint32_t     votepower_age_update_height = 0;

        uint32_t     unfreeze_block_num          = 0;
        uint32_t     freeze_block_num            = 0;
        
        bool is_has_quited = false;

        EOSLIB_SERIALIZE( vote_info_item, 
            ( vote )( votepower )( votepower_age )( votepower_age_update_height )
            ( unfreeze_block_num )( freeze_block_num )( is_has_quited ) )
    };

    struct vote_info {
        account_name bpname                = 0;

        asset        vote                  = asset{ 0 };
        asset        votepower             = asset{ 0 };
        int64_t      voteage               = 0; // asset.amount * block height
        uint32_t     voteage_update_height = 0;

        std::vector<vote_info_item> static_votes;

        void append_static_vote( const asset&   stake,
                                 const uint64_t power,
                                 const uint32_t lock_block_num,
                                 const uint32_t curr_block_num ) {
            static_votes.push_back(vote_info_item{
                stake,
                stake * power,
                0,
                curr_block_num,
                curr_block_num + lock_block_num,
                lock_block_num
            });
        }

        uint64_t primary_key() const { return bpname; }

        EOSLIB_SERIALIZE( vote_info, ( bpname )( vote )( votepower )( voteage )( voteage_update_height )( static_votes ) )
    };

    struct voter_info {
        account_name voter       = 0;
        asset        voteall     = asset{ 0 };
        std::vector<account_name> bpnames;

        uint64_t primary_key() const { return voter; }

        EOSLIB_SERIALIZE( voter_info, ( voter )( voteall )( bpnames ) )
    };

    struct bp_opened {
        account_name bpname = 0;

        uint64_t primary_key() const { return bpname; }

        EOSLIB_SERIALIZE( bp_opened, ( bpname ) )
    };

    struct freeze_info {
        asset        staked         = asset{ 0 };
        asset        unstaking      = asset{ 0 };
        account_name voter          = 0;
        uint32_t     unstake_height = 0;

        uint64_t primary_key() const { return voter; }

        EOSLIB_SERIALIZE( freeze_info, ( staked )( unstaking )( voter )( unstake_height ) )
    };

    struct vote4ram_info {
        account_name voter  = 0;
        asset        staked = asset( 0 );

        uint64_t primary_key() const { return voter; }

        EOSLIB_SERIALIZE( vote4ram_info, ( voter )( staked ) )
    };

    struct bp_info {
        account_name name;
        account_name invite;
        public_key   block_signing_key;
        int64_t      total_staked          = 0;
        asset        rewards_pool          = asset( 0 );
        asset        rewards_block         = asset( 0 );
        int64_t      total_voteage         = 0; // asset.amount * block height
        uint32_t     voteage_update_height = current_block_num();
        std::string  url;

        asset        pawn  = asset{}; // bp pawns

        bool emergency = false;
        bool isactive  = true;

        uint64_t primary_key() const { return name; }

        void update( public_key key, std::string u ) {
            block_signing_key = key;
            url               = u;
        }
        void deactivate() { isactive = false; }
        EOSLIB_SERIALIZE( bp_info,
                          ( name )( invite )( block_signing_key )
                          ( total_staked )( rewards_pool )(rewards_block )( total_voteage )
                          ( voteage_update_height )( url )( emergency )( isactive )( pawn ) )
    };

    struct producer {
        account_name bpname;
        uint32_t     amount = 0;
    };

    struct schedule_info {
        uint64_t version;
        uint32_t block_height;
        producer producers[NUM_OF_TOP_BPS];

        uint64_t primary_key() const { return version; }

        EOSLIB_SERIALIZE( schedule_info, ( version )( block_height )( producers ) )
    };

    struct vid_info {
        account_name vid;
        account_name account;

        uint64_t primary_key() const { return vid; }

        EOSLIB_SERIALIZE( vid_info, ( vid )( account ) )
    };

    struct vid_state {
        uint32_t block_num;
        uint32_t vid_count;

        vector<vid_info> vids;

        uint64_t primary_key() const { return static_cast<uint32_t>(block_num); }

        EOSLIB_SERIALIZE( vid_state, ( block_num )( vid_count )( vids ) )
    };

    struct vid_params {
        uint64_t id                     = 0;
        uint64_t vid_reward_coefficient = 0;
        asset    reward_per_vid         = asset{0};

        uint64_t primary_key() const { return id; }

        EOSLIB_SERIALIZE( vid_params, ( id )( vid_reward_coefficient )( reward_per_vid ) )
    };

    struct vote_rank_node {
        account_name bp                    = 0;
        int64_t      staked                = 0;
        int64_t      voteage               = 0;
        uint32_t     voteage_update_height = 0;

        uint32_t     start_height = 0;

        uint64_t primary_key() const { return bp; }

        EOSLIB_SERIALIZE( vote_rank_node, ( bp )( staked )( voteage )( voteage_update_height )( start_height ) )
    };
    

    typedef eosio::multi_index<N( freezed ), freeze_info>       freeze_table;
    typedef eosio::multi_index<N( votes ), vote_info>           votes_table;
    typedef eosio::multi_index<N( voterinfo ), voter_info>      voter_table;
    typedef eosio::multi_index<N( votes4ram ), vote_info>       votes4ram_table;
    typedef eosio::multi_index<N( vote4ramsum ), vote4ram_info> vote4ramsum_table;
    typedef eosio::multi_index<N( bps ), bp_info>               bps_table;
    typedef eosio::multi_index<N( bpopened ), bp_opened>        bpopened_table;
    typedef eosio::multi_index<N( schedules ), schedule_info>   schedules_table;
    typedef eosio::multi_index<N( vids ), vid_info>             vids_table;

    typedef eosio::multi_index<N( vidstates ), vid_state>       vid_state_table;
    typedef eosio::multi_index<N( vidparam ), vid_params>       vid_params_table;

    typedef eosio::multi_index<N( voterank ), vote_rank_node>  vote_ranks_table;

    bps_table        _bps;
    freeze_table     _freezes;
    vids_table       _vids;
    vid_state_table  _vid_stats;
    vid_params_table _vid_params;
    voter_table      _voters;

    void update_elected_bps( const uint32_t curr_block_num );

    void reward_bps_and_user( account_name block_producers[],
                              account_name producer,
                              const asset& for_produce, const asset& for_bp, const asset& for_user );

    inline bool is_genesis_init_bp( account_name bpname ) {
        const auto name_str = eosio::name{bpname}.to_string();
        return name_str.find("bacc.") == 0;
    }

    // defind in delegate_bandwidth.cpp
    void changebw( account_name from,
                   account_name receiver,
                   asset        stake_net_quantity,
                   asset        stake_cpu_quantity,
                   bool         transfer );

    inline void update_vote_rank( const account_name rank_name, 
                                  const account_name bp, 
                                  const int64_t staked,
                                  const uint32_t curr_block_num,
                                  const uint32_t current_rank_start_height ) {
        vote_ranks_table vr_table( _self, rank_name );
        const auto vr_itr = vr_table.find( bp );
        if( vr_itr == vr_table.end() ) {
            vr_table.emplace( _self, [&]( vote_rank_node& v ) { 
                v.bp = bp;
                v.staked = staked;
                v.voteage = 0;
                v.voteage_update_height = curr_block_num;

                v.start_height = current_rank_start_height;
            } );
        } else {
            vr_table.modify( vr_itr, _self, [&]( vote_rank_node& v ) {
                if ( v.start_height == current_rank_start_height ){
                    const auto total_voteage_addon = v.staked * ( curr_block_num - v.voteage_update_height );
                    v.staked  += staked;
                    v.voteage += total_voteage_addon;
                    v.voteage_update_height = curr_block_num;
                } else {
                    // update
                    const auto total_voteage_from_start = v.staked * ( curr_block_num - current_rank_start_height );
                    v.staked  += staked;
                    v.voteage = total_voteage_from_start;
                    v.voteage_update_height = curr_block_num;

                    v.start_height = current_rank_start_height;
                }
            } );
        }
    }

    void modify_bp_vote( const bp_info& bp, const int64_t stake, const uint32_t curr_block_num  ) {
        const auto per_vote_rank_num = 7 * BLOCK_NUM_PER_DAY;
        const auto current_rank_start_height = curr_block_num - (( curr_block_num ) % ( per_vote_rank_num )) + 1;
        const auto total_voteage_addon = bp.total_staked * ( curr_block_num - bp.voteage_update_height );

        // Update BP info
        _bps.modify( bp, 0, [&]( bp_info& b ) {
            b.total_voteage += total_voteage_addon;
            b.voteage_update_height = curr_block_num;
            b.total_staked += stake;
        } );
    }

    void update_voter_info( const account_name voter, const account_name bpname, const asset& stake_change ) {
        auto vt = _voters.find( voter );
        if( vt == _voters.end() ) {
            _voters.emplace( voter, [&]( voter_info& v ) { 
                v.voter = voter;
                v.voteall = stake_change;
            } );
        } else {
            eosio_assert( (vt->voteall + stake_change) >= asset{0}, "cant to voteall < 0" );
            _voters.modify( vt, voter, [&]( voter_info& v ) {
                v.voteall += stake_change;
            } );
        }
    }

  public:
    // @abi action
    void updatebp( const account_name bpname,
                   const account_name invite,
                   const public_key   producer_key,
                   const std::string& url );

    // @abi action
    void freeze( const account_name voter, const asset stake );

    // @abi action
    void unfreeze( const account_name voter );

    // @abi action
    void vote( const account_name voter, const account_name bpname, const asset stake );

    // @abi action
    void votebyreward( const account_name voter, const account_name bpname );

    // @abi action
    void votestatic( const account_name voter, const account_name bpname, const asset stake, const uint32_t typ );
    
    // @abi action
    void quitsvote( const account_name voter, const account_name bpname, const uint32_t idx );

    // @abi action
    void vote4ram( const account_name voter, const account_name bpname, const asset stake );

    // @abi action
    void claim( const account_name voter, const account_name bpname );

    // @abi action
    void onblock( const block_timestamp,
                  const account_name bpname,
                  const uint16_t,
                  const block_id_type,
                  const checksum256,
                  const checksum256,
                  const uint32_t schedule_version );

    // @abi action
    void setparams( const eosio::blockchain_parameters& params );
    // @abi action
    void removebp( account_name producer );

    // @abi action
    void openbp( account_name producer );
    // @abi action
    void banbp( account_name producer );
    // @abi action
    void bppawn( account_name bpname, asset pawn );
    // @abi action
    void bpredeem( account_name bpname );
    // @abi action
    void bpclaim( account_name bpname );

    // @abi action
    void setvidparam( uint64_t coefficient, asset reward_per_vid );

    // native action
    // @abi action
    //,authority owner,authority active
    void newaccount( account_name creator, account_name name, authority owner, authority active, account_name vid );
    // @abi action
    // account_name account,permission_name permission,permission_name
    // parent,authority auth
    void updateauth( account_name account, permission_name permission, permission_name parent, authority auth );
    // @abi action
    void deleteauth( account_name account, permission_name permission );
    // @abi action
    void linkauth( account_name account, account_name code, action_name type, permission_name requirement );
    // @abi action
    void unlinkauth( account_name account, account_name code, action_name type );
    // @abi action
    void canceldelay( permission_level canceling_auth, transaction_id_type trx_id );
    // @abi action
    void onerror( uint128_t sender_id, bytes sent_trx );
    // @abi action
    void setconfig( account_name typ, int64_t num, account_name key, asset fee );
    // @abi action
    void setcode( account_name account, uint8_t vmtype, uint8_t vmversion, bytes code );
    // @abi action
    void setfee( account_name account,
                 action_name  action,
                 asset        fee,
                 uint32_t     cpu_limit,
                 uint32_t     net_limit,
                 uint32_t     ram_limit );
    // @abi action
    void setabi( account_name account, bytes abi );
    // @abi action
    void setram( const account_name account, const asset stake );

#if CONTRACT_RESOURCE_MODEL == RESOURCE_MODEL_DELEGATE
    // @abi action
    void delegatebw( account_name from,
                     account_name receiver,
                     asset        stake_net_quantity,
                     asset        stake_cpu_quantity,
                     bool         transfer );
    // @abi action
    void undelegatebw( account_name from,
                       account_name receiver,
                       asset        unstake_net_quantity,
                       asset        unstake_cpu_quantity );
    // @abi action
    void refund( account_name owner );
#endif
};
}; // namespace noc

EOSIO_ABI( noc::system_contract,
           ( updatebp )
           ( freeze )
           ( unfreeze )
           ( vote )( vote4ram )( votestatic )( quitsvote )( votebyreward )
           ( claim )( onblock )( setparams )( removebp )
           ( newaccount )( updateauth )( deleteauth )( linkauth )( unlinkauth )
           ( canceldelay )( onerror )( setconfig )
           ( setcode )( setfee )( setabi )
           ( banbp )( openbp )( bppawn )( bpredeem )( bpclaim )( setvidparam )( setram )
#if CONTRACT_RESOURCE_MODEL == RESOURCE_MODEL_DELEGATE
             ( delegatebw )( undelegatebw )( refund )
#endif
)
