/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "noc.token.hpp"

namespace eosio {

// TODO use general freeze
struct freeze_info {
    asset        staked         = asset{ 0 };
    asset        unstaking      = asset{ 0 };
    account_name voter          = 0;
    uint32_t     unstake_height = 0;

    uint64_t primary_key() const { return voter; }

    EOSLIB_SERIALIZE( freeze_info, ( staked )( unstaking )( voter )( unstake_height ) )
};
typedef eosio::multi_index<N( freezed ), freeze_info> freeze_table;

struct voter_info {
    account_name voter       = 0;
    asset        voteall     = asset{ 0 };
    std::vector<account_name> bpnames;

    uint64_t primary_key() const { return voter; }

    EOSLIB_SERIALIZE( voter_info, ( voter )( voteall )( bpnames ) )
};
typedef eosio::multi_index<N( voterinfo ), voter_info> voter_table;

void token::create( account_name issuer,
                    asset        maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");
    eosio_assert( sym != CORE_SYMBOL, "not allow create core symbol token by token contract");

    stats statstable( _self, sym.name() );
    auto existing = statstable.find( sym.name() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void token::issue( account_name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.name();
    stats statstable( _self, sym_name );
    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, 0, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
       SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
    }
}

void token::transfer( account_name from,
                      account_name to,
                      asset        quantity,
                      string       memo )
{
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );


    sub_balance( from, quantity );
    add_balance( to, quantity, from );
}

void token::fee( account_name payer, asset quantity ){
   const auto fee_account = N(bacc.fee);

   require_auth( payer );

   auto sym = quantity.symbol.name();
   stats statstable( _self, sym );
   const auto& st = statstable.get( sym );

   eosio_assert( quantity.is_valid(), "invalid quantity" );
   eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
   eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

   sub_balance( payer, quantity );
   add_balance( fee_account, quantity, payer );
}

void token::sub_balance( account_name owner, asset value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );

   int64_t freeze_core_token_amount = 0;
   freeze_table freeze_tbl( config::system_account_name, config::system_account_name );
   voter_table voter_tbl( config::system_account_name, config::system_account_name );
   auto vt = voter_tbl.find( owner );
   auto fts = freeze_tbl.find( owner );
   if ( fts != freeze_tbl.end() ) {
      auto vtall = ( (vt == voter_tbl.end()) ? asset{ 0 } : vt->voteall );
      freeze_core_token_amount = (fts->staked + fts->unstaking + vtall).amount;
      if( freeze_core_token_amount > 0 ) {
         // give a detail reason if by freeze
         eosio_assert( (from.balance.amount >= (value.amount + freeze_core_token_amount)), 
            "overdrawn balance by freeze lots tokens" );
      }
   }

   eosio_assert( from.balance.amount >= (value.amount + freeze_core_token_amount), "overdrawn balance" );

   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void token::add_balance( account_name owner, asset value, account_name ram_payer )
{
   accounts to_acnts( _self, owner );
   
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

} /// namespace eosio

EOSIO_ABI( eosio::token, (create)(issue)(transfer)(fee) )
