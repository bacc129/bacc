#include "noc.system.hpp"

namespace noc {
void system_contract::updateauth( account_name, permission_name, permission_name, authority ) {}

void system_contract::deleteauth( account_name, permission_name ) {}

void system_contract::linkauth( account_name, account_name, action_name, permission_name ) {}

void system_contract::unlinkauth( account_name, account_name, action_name ) {}

void system_contract::canceldelay( permission_level, transaction_id_type ) {}

void system_contract::onerror( uint128_t, bytes ) {}

void system_contract::setconfig( account_name, int64_t, account_name, asset ) {}

void system_contract::setcode( account_name, uint8_t, uint8_t, bytes ) {}

void system_contract::setfee( account_name, action_name, asset, uint32_t, uint32_t, uint32_t ) {}

void system_contract::setabi( account_name, bytes ) {}
} // namespace noc
