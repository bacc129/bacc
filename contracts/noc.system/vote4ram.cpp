#include "noc.system.hpp"

namespace noc {
void system_contract::vote4ram( const account_name voter, const account_name bpname, const asset stake ) {
    require_auth( voter );
    set_need_check_ram_limit( voter );
}

void system_contract::setram( const account_name account, const asset stake ) {
    require_auth( _self );

    vote4ramsum_table vote4ramsum_tbl( _self, _self );
    auto vtss = vote4ramsum_tbl.find( account );
    if( vtss == vote4ramsum_tbl.end() ) {
        vote4ramsum_tbl.emplace( account, [&]( vote4ram_info& v ) {
            v.voter = account;
            v.staked = stake; // for first vote all staked is stake
         } );
    } else {
        vote4ramsum_tbl.modify( vtss, 0, [&]( vote4ram_info& v ) { 
            v.staked = stake; 
        } );
    }

    set_need_check_ram_limit( account );
}

} // namespace noc
