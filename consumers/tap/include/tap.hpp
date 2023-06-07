#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

using namespace std;
using namespace eosio;

#define ORCHESTRATOR_CONTRACT_NAME "router111111"
#define NEW_BADGE_ISSUANCE_NOTIFICATION ORCHESTRATOR_CONTRACT_NAME"::notifyachiev"
#define NEW_BADGE_SUBSCRIPTION_NOTIFICATION ORCHESTRATOR_CONTRACT_NAME"::addnotify"

CONTRACT tap : public contract {
  public:
    using contract::contract;


    [[eosio::on_notify(NEW_BADGE_SUBSCRIPTION_NOTIFICATION)]] void notifyinit(
      name org,
      name badge_contract,
      name badge_name,
      name notify_account,
      string memo, 
      uint64_t badge_id, 
      string offchain_lookup_data,
      string onchain_lookup_data,
      uint32_t rarity_counts);

    [[eosio::on_notify(NEW_BADGE_ISSUANCE_NOTIFICATION)]] void notifyachiev (
      name org, 
      name badge_contract, 
      name badge_name,
      name account, 
      name from,
      uint8_t count,
      string memo,
      uint64_t badge_id,  
      vector<name> notify_accounts);

    ACTION pause(name org, name issuing_contract, name assetname);
    ACTION resume(name org, name issuing_contract, name assetname);
    ACTION timebound(name org, name issuing_contract, name assetname, time_point_sec start_time, time_point_sec end_time);
    ACTION supplybound(name org, name issuing_contract, name assetname, uint64_t supplycap);
    ACTION removesb(name org, name issuing_contract, name assetname);
    ACTION removetb(name org, name issuing_contract, name assetname);
  private:
    TABLE tapstatus {
      uint64_t badge_id;
      bool  pause;
      bool time_bound;
      time_point start_time;
      time_point end_time;
      bool supply_bound;
      uint64_t supplycap;
      uint64_t current_supply;
      auto primary_key() const { return badge_id; }
    };
    typedef multi_index<name("tapstatus"), tapstatus> tapstatus_table;

    TABLE badge {
      uint64_t badge_id;
      name badge_contract;
      name badge_name;
      vector<name> notify_accounts;
      string offchain_lookup_data;
      string onchain_lookup_data;
      uint32_t rarity_counts;
      auto primary_key() const {return badge_id; }
      uint128_t contract_badge_key() const {
        return ((uint128_t) badge_contract.value) << 64 | badge_name.value;
      }
    };
    typedef multi_index<name("badge"), badge,
    indexed_by<name("contractbadge"), const_mem_fun<badge, uint128_t, &badge::contract_badge_key>>    
    > badge_table;

    uint64_t get_badge_id (name org, name issuing_contract, name assetname) {
      badge_table _badge(name(ORCHESTRATOR_CONTRACT_NAME), org.value);
      auto contract_badge_index = _badge.get_index<name("contractbadge")>();
      uint128_t contract_badge_key = ((uint128_t) issuing_contract.value) << 64 | assetname.value;
      auto contract_badge_iterator = contract_badge_index.require_find (contract_badge_key, "Could not find Contract, badge ");
      return contract_badge_iterator->badge_id;
    }
};
