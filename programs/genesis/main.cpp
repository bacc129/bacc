#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <fc/variant_object.hpp>

#include <eosio/chain/genesis_state.hpp>
#include <eosio/chain/name.hpp>
#include <eosio/chain/config.hpp>

#include <fstream>

//using namespace eosio;
using namespace eosio::chain;

struct key_map {
   std::map<account_name, fc::crypto::private_key> keymap;

   void add_init_acc(
         eosio::chain::genesis_state &gs,
         const account_name &name,
         const uint64_t eos,
         const private_key_type key = fc::crypto::private_key::generate<fc::ecc::private_key_shim>()) {
      const auto tu = eosio::chain::account_tuple{
            key.get_public_key(), eosio::chain::asset(eos * 10000), name
      };

      keymap[name] = key;
      gs.initial_account_list.push_back(tu);
   }
};
FC_REFLECT(key_map, ( keymap ))



int main( int argc, const char **argv ) {
   eosio::chain::genesis_state gs;
   const std::string path = "./genesis.json";
   const std::string activeacc = "./activeacc.json";
   key_map my_keymap;
   key_map my_sign_keymap;
   std::ofstream out("./config.ini");

   // NOC5muUziYrETi5b6G2Ev91dCBrEm3qir7PK4S2qSFqfqcmouyzCr
   const auto bp_private_key = fc::crypto::private_key(std::string("5KYQvUwt6vMpLJxqg4jSQNkuRfktDHtYDp8LPoBpYo8emvS1GfG"));

   // NOC7R82SaGaJubv23GwXHyKT4qDCVXi66qkQrnjwmBUvdA4dyzEPG
   const auto bpsign_private_key = fc::crypto::private_key(std::string("5JfjatHRwbmY8SfptFRxHnYUctfnuaxANTGDYUtkfrrBDgkh3hB"));


   for( int i = 0; i < config::max_producers; i++ ) {

      auto pub_key = bp_private_key.get_public_key();
      eosio::chain::account_tuple tu;
      tu.key = pub_key;
      tu.asset = eosio::chain::asset(0);
      std::string name("bacc.initbp");
      char mark = 'a' + i;
      name.append(1u, mark);
      tu.name = string_to_name(name.c_str());
      gs.initial_account_list.push_back(tu);

      auto sig_pub_key = bpsign_private_key.get_public_key();
      out << "producer-name = " << name << "\n";
      out << "private-key = [\"" << string(sig_pub_key) << "\",\"" << string(bpsign_private_key) << "\"]\n";

      producer_tuple tp;
      tp.bpkey = sig_pub_key;
      tp.name = string_to_name(name.c_str());
      gs.initial_producer_list.push_back(tp);
      my_keymap.keymap[string_to_name(name.c_str())] = bp_private_key;
      my_sign_keymap.keymap[string_to_name(name.c_str())] = bpsign_private_key;
   }

   const auto private_key = bp_private_key;

   my_keymap.add_init_acc(gs, N(bacc.init),     2844*10000,  private_key);
   my_keymap.add_init_acc(gs, N(bacc.dev),      0*10000,     private_key);
   my_keymap.add_init_acc(gs, N(bacc.better),   0*10000,     private_key);
   my_keymap.add_init_acc(gs, N(bacc.vpool),    0*10000,     private_key);
   my_keymap.add_init_acc(gs, N(bacc.pawn),     0*10000,     private_key);
   my_keymap.add_init_acc(gs, N(bacc.pow),      0*10000,     private_key);

   // for test
   my_keymap.add_init_acc(gs, N(bacc.test), 1000*10000,  private_key);
   my_keymap.add_init_acc(gs, config::chain_config_name, 0*10000, private_key);

   // test accounts
   for( int j = 0; j < 20; ++j ) {
      std::string n = "test.";
      const char mark = static_cast<char>('a' + j);
      n += mark;
      my_keymap.add_init_acc(
            gs,
            string_to_name(n.c_str()),
            10 * 10000, private_key);
   }

   out.close();

   const std::string keypath = "./key.json";
   const std::string sigkeypath = "./sigkey.json";
   const std::string configini = "./config.ini";

   fc::json::save_to_file<eosio::chain::genesis_state>(gs, path, true);
   fc::json::save_to_file<std::vector<account_tuple>>(gs.initial_account_list, activeacc, true);
   fc::json::save_to_file<key_map>(my_keymap, keypath, true);
   fc::json::save_to_file<key_map>(my_sign_keymap, sigkeypath, true);

   return 0;
}
