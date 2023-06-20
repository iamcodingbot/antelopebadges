#include <eosio/eosio.hpp>

using namespace std;
using namespace eosio;

#define SIMPLEBADGE_CONTRACT_NAME "basicissue11"
#define CLAIMASSET_CONTRACT_NAME "claimasset11"
#define NOTIFICATION_CONTRACT_NAME "notification"
#define ORCHESTRATOR_CONTRACT_NAME "router111111"

#define NEW_BADGE_ISSUANCE_NOTIFICATION ORCHESTRATOR_CONTRACT_NAME"::notifyachiev"

CONTRACT andemitter : public contract {
  public:
    using contract::contract;
    struct asset_contract_name {
      name issuing_contract;
      name asset_name;

      bool operator<(const asset_contract_name& ad) const {
        if (this->issuing_contract == ad.issuing_contract) {
          return (this->asset_name.to_string() < ad.asset_name.to_string());
        } else {
          return (this->issuing_contract.to_string() < ad.issuing_contract.to_string());
        }
      }
    };

    [[eosio::on_notify(NEW_BADGE_ISSUANCE_NOTIFICATION)]] void notifyachiev (
      name org, 
      name badge_name,
      name account, 
      name from,
      uint64_t count,
      string memo,
      uint64_t badge_id,  
      vector<name> notify_accounts);

    ACTION newemission (name org,
      name emission_name,
      std::map<name, uint64_t> emitter_criteria,
      std::map<asset_contract_name, uint64_t> emit_assets,
      bool cyclic);

    ACTION activate (name org, name emission_name);

    ACTION deactivate (name org, name emission_name);

    ACTION givesimple (name org, name to, name badge, uint64_t amount, string memo);

    ACTION addclaimer (name org, name account, name assetname, uint64_t account_cap, string memo);


  private:
    TABLE emissions {
      name emission_name;
      std::map<name, uint64_t> emitter_criteria;
      std::map<asset_contract_name, uint64_t> emit_assets;
      name status; // INIT, ACTIVATE, DEACTIVATE
      bool cyclic;
      auto primary_key() const { return emission_name.value; }
    };
    typedef multi_index<name("emissions"), emissions> emissions_table;

    TABLE activelookup {
      uint64_t badge_id;
      vector<name> active_emissions;
      auto primary_key() const { return badge_id; }
    };
    typedef multi_index<name("activelookup"), activelookup> activelookup_table;

    TABLE accounts {
      uint64_t id;
      name account;
      name emission_name;
      uint8_t emission_status;
      std::map<name, uint64_t> expanded_emitter_status;
      auto primary_key() const { return id; }
      uint128_t account_emission_name_key() const {
        return ((uint128_t) account.value) << 64 | emission_name.value;
      }
      uint64_t emission_name_key() const {
        return emission_name.value;
      } // TODO are these 2 keys needed?
    };
    typedef multi_index<name("accounts"), accounts,
    indexed_by<name("accntemssn"), const_mem_fun<accounts, uint128_t, &accounts::account_emission_name_key>>,
    indexed_by<name("emissionkey"), const_mem_fun<accounts, uint64_t, &accounts::emission_name_key>>    
    > accounts_table;

    TABLE badge {
      uint64_t badge_id;
      name badge_name;
      vector<name> notify_accounts;
      string offchain_lookup_data;
      string onchain_lookup_data;
      uint64_t rarity_counts;
      auto primary_key() const {return badge_id; }
      uint64_t badge_key() const {
        return badge_name.value;
      }
    };
    typedef multi_index<name("badge"), badge,
    indexed_by<name("badgename"), const_mem_fun<badge, uint64_t, &badge::badge_key>>    
    > badge_table;

    struct issue_args {
      name org;
      name to;
      name badge; 
      uint64_t amount;
      string memo;
    };

    struct addclaimer_args {
      name org;
      name account;
      name assetname; 
      uint64_t account_cap;
      string memo;
    };

    enum short_emission_status: uint8_t {
      CYCLIC_IN_PROGRESS = 1,
      NON_CYCLIC_IN_PROGRESS = 2,
      NON_CYCLIC_EMITTED = 3
    };
    void invoke_action ( name org, std::map<asset_contract_name, uint64_t> emit_assets ,name to, uint8_t emit_factor) {

      if(emit_factor == 0) {
        return;
      }
      
      for (const auto& [key, value] : emit_assets) {
        if(key.issuing_contract == name(SIMPLEBADGE_CONTRACT_NAME)) {
          action {
            permission_level{get_self(), name("active")},
            name(get_self()),
            name("givesimple"),
            issue_args {
              .org = org,
              .to = to,
              .badge = key.asset_name,
              .amount = value * emit_factor,
              .memo = "issued from rollup consumer"}
          }.send(); 
        } else if (key.issuing_contract == name(CLAIMASSET_CONTRACT_NAME)) {
          action {
            permission_level{get_self(), name("active")},
            name(get_self()),
            name("addclaimer"),
            addclaimer_args {
              .org = org,
              .account = to,
              .assetname = key.asset_name,
              .account_cap = value * emit_factor,
              .memo = "issued from rollup consumer" }
          }.send(); 
        }

      }

    }

    uint64_t get_badge_id (name org, name assetname) {
      badge_table _badge(name(ORCHESTRATOR_CONTRACT_NAME), org.value);
      auto badge_index = _badge.get_index<name("badgename")>();
      auto badge_iterator = badge_index.find (assetname.value);
      check(badge_iterator->badge_name == assetname, "asset not found");
      return badge_iterator->badge_id;
    }

    uint8_t emit_factor (std::map<name, uint64_t> emitter_status, std::map<name, uint64_t> emitter_criteria) {
      uint8_t key_factor = 0;
      uint8_t max_factor = 0;
      for (const auto& [key, value] : emitter_criteria) {
      key_factor = emitter_status[key] / emitter_criteria[key];
        if(key_factor > max_factor) {
          max_factor = key_factor;
        }
      }
      return max_factor;
    }

    uint8_t real_emit_factor (bool cyclic, uint8_t emit_factor) {
      if(emit_factor > 0 && !cyclic) {
        return 1;
      } 
      return emit_factor;
    }

  std::map<name, uint64_t> update_expanded_emitter_status (std::map<name, uint64_t> expanded_emitter_status, 
      std::map<name, uint64_t> emitter_criteria,
      uint8_t emit_factor 
      ) {
      std::map<name, uint64_t> updated_expanded_emitter_status;
      for (const auto& [key, value] : expanded_emitter_status) { 
        updated_expanded_emitter_status[key]= expanded_emitter_status[key] - emit_factor * emitter_criteria[key];
      }
      return updated_expanded_emitter_status;
    }

  void emitters (name org, name emission_name, name account, name asset_name, uint8_t count) {

      emissions_table _emissions(_self, org.value);
      auto emissions_itr = _emissions.require_find(emission_name.value, "Somethings wrong");

      accounts_table _accounts(_self, org.value);
      auto account_emission_index = _accounts.get_index<name("accntemssn")>();
      uint128_t account_emission_key = ((uint128_t) account.value) << 64 | emission_name.value;
      auto account_emission_iterator = account_emission_index.find (account_emission_key);

      bool present_in_accounts = account_emission_iterator != account_emission_index.end() && 
        account_emission_iterator->account == account && 
        account_emission_iterator->emission_name == emission_name;
      
      if (present_in_accounts && account_emission_iterator->emission_status == NON_CYCLIC_EMITTED) {
        return;
      }

      std::map<name, uint64_t> expanded_emitter_status;
      std::map<name, uint64_t> updated_expanded_emitter_status;
      bool cyclic = emissions_itr->cyclic;
      uint8_t emission_factor;
      if (present_in_accounts) {
        expanded_emitter_status = account_emission_iterator->expanded_emitter_status;
        expanded_emitter_status[asset_name] = (expanded_emitter_status.contains(asset_name)) 
          ? expanded_emitter_status[asset_name] + count 
          : count;

        uint8_t factor = emit_factor(expanded_emitter_status, emissions_itr->emitter_criteria);
        emission_factor = real_emit_factor (cyclic, factor);
       
        updated_expanded_emitter_status = update_expanded_emitter_status (expanded_emitter_status, emissions_itr->emitter_criteria, emission_factor);
        if (emission_factor > 0 && !cyclic) {
            account_emission_index.modify(account_emission_iterator, get_self(), [&](auto& row){
              row.emission_status = NON_CYCLIC_EMITTED;
              row.expanded_emitter_status = updated_expanded_emitter_status;
            });
        }
        if (emission_factor == 0 && !cyclic) {
            account_emission_index.modify(account_emission_iterator, get_self(), [&](auto& row){
              row.emission_status = NON_CYCLIC_IN_PROGRESS;
              row.expanded_emitter_status = updated_expanded_emitter_status;
            });
        }
        if (emission_factor > 0 && cyclic) {
            account_emission_index.modify(account_emission_iterator, get_self(), [&](auto& row){
              row.emission_status = CYCLIC_IN_PROGRESS;
              row.expanded_emitter_status = updated_expanded_emitter_status;
            });
        }
        if(emission_factor == 0 && cyclic) {
            account_emission_index.modify(account_emission_iterator, get_self(), [&](auto& row){
              row.emission_status = CYCLIC_IN_PROGRESS;
              row.expanded_emitter_status = updated_expanded_emitter_status;
            });
        }

      } else {
        expanded_emitter_status[asset_name] = count;
        uint8_t factor = emit_factor(expanded_emitter_status, emissions_itr->emitter_criteria);
        emission_factor = real_emit_factor (cyclic, factor);
        updated_expanded_emitter_status = update_expanded_emitter_status (expanded_emitter_status, emissions_itr->emitter_criteria, emission_factor);

        if (emission_factor > 0 && !cyclic) {
          _accounts.emplace(get_self(), [&](auto& row){
            row.emission_status = NON_CYCLIC_EMITTED;
            row.id = _accounts.available_primary_key();
            row.emission_name = emission_name;
            row.account = account;
            row.expanded_emitter_status = updated_expanded_emitter_status;
          });

        }
        if (emission_factor == 0 && !cyclic) {
          _accounts.emplace(get_self(), [&](auto& row){
            row.emission_status = NON_CYCLIC_IN_PROGRESS;
            row.id = _accounts.available_primary_key();
            row.emission_name = emission_name;
            row.account = account;
            row.expanded_emitter_status = updated_expanded_emitter_status;
          });

        }
        if (emission_factor > 0 && cyclic) {
          _accounts.emplace(get_self(), [&](auto& row){
            row.emission_status = CYCLIC_IN_PROGRESS;
            row.id = _accounts.available_primary_key();
            row.emission_name = emission_name;
            row.account = account;
            row.expanded_emitter_status = updated_expanded_emitter_status;
          });

        }
        if(emission_factor == 0 && cyclic) {
          _accounts.emplace(get_self(), [&](auto& row){
            row.emission_status = CYCLIC_IN_PROGRESS;
            row.id = _accounts.available_primary_key();
            row.emission_name = emission_name;
            row.account = account;
            row.expanded_emitter_status = updated_expanded_emitter_status;
          });

        }
        
      }
      
      invoke_action(org,  emissions_itr->emit_assets, account, emission_factor);

      
    }
};
