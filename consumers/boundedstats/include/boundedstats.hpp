#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

using namespace std;
using namespace eosio;

#define ORCHESTRATOR_CONTRACT "orchzzzzzzzz"
#define BOUNDED_AGG_CONTRACT "baggzzzzzzzz"
#define AUTHORITY_CONTRACT "authorityzzz"
#define NEW_BADGE_ISSUANCE_NOTIFICATION ORCHESTRATOR_CONTRACT"::notifyachiev"

CONTRACT boundedstats : public contract {
  public:
    using contract::contract;

    [[eosio::on_notify(NEW_BADGE_ISSUANCE_NOTIFICATION)]] void notifyachiev(asset badge_asset, 
        name from, 
        name to, 
        string memo, 
        vector<name> notify_accounts);


    ACTION settings(symbol agg_symbol, uint64_t seq_id, symbol badge_symbol, uint64_t max_no_of_ranks);

  private:

    TABLE  statssetting {
        uint64_t badge_agg_seq_id;
        uint64_t max_no_of_ranks;
        uint64_t current_no_of_ranks;
        uint64_t primary_key() const { return badge_agg_seq_id; }
    };
    typedef multi_index<"statssetting"_n, statssetting> statssetting_table;

    TABLE counts {
        uint64_t badge_agg_seq_id;
        uint64_t total_recipients;
        uint64_t total_issued;
        uint64_t primary_key() const { return badge_agg_seq_id; }
    };
    typedef multi_index<"counts"_n, counts> counts_table;

    TABLE ranks {
        vector<name> accounts;
        uint64_t balance;
        uint64_t primary_key() const { return balance; }
    };
    typedef multi_index<"ranks"_n, ranks> ranks_table;

    TABLE badgeaggseq {
        uint64_t badge_agg_seq_id; // Primary key: Unique ID for each badge-sequence association
        symbol agg_symbol;         // The aggregation symbol associated with this badge
        uint64_t seq_id;           // The sequence ID this badge is associated with
        symbol badge_symbol;       // The symbol representing the badge
        name badge_status;         // The status of the badge (e.g., "active")
        name seq_status;           // The status of the sequence copied from the agg table

        uint64_t primary_key() const { return badge_agg_seq_id; }
        uint128_t by_agg_seq() const { return combine_keys(agg_symbol.code().raw(), seq_id); }
        checksum256 by_badge_status() const {
            // Example hashing for badge status; adjust according to actual implementation needs
            auto data = badge_symbol.code().to_string() + badge_status.to_string() + seq_status.to_string();
            return sha256(data.data(), data.size());
        }

        static uint128_t combine_keys(uint64_t a, uint64_t b) {
            // Combines two 64-bit integers into a single 128-bit value for indexing purposes
            return (static_cast<uint128_t>(a) << 64) | b;
        }
    };
    typedef eosio::multi_index<"badgeaggseqs"_n, badgeaggseq,
        eosio::indexed_by<"byaggseq"_n, eosio::const_mem_fun<badgeaggseq, uint128_t, &badgeaggseq::by_agg_seq>>,
        eosio::indexed_by<"bybadgestat"_n, eosio::const_mem_fun<badgeaggseq, checksum256, &badgeaggseq::by_badge_status>>
    > badgeaggseq_table;
    
    TABLE achievements {
        uint64_t badge_agg_seq_id;
        uint64_t count;

        uint64_t primary_key() const { return badge_agg_seq_id; }
    };
    typedef eosio::multi_index<"achievements"_n, achievements> achievements_table;

        // Define the structure of the table
    TABLE orgcode {
        name org;         // Organization identifier, used as primary key
        name org_code;    // Converted org_code, ensuring uniqueness and specific format

        // Specify the primary key
        auto primary_key() const { return org.value; }

        // Specify a secondary index for org_code to ensure its uniqueness
        uint64_t by_org_code() const { return org_code.value; }
    };

    // Declare the table
    typedef eosio::multi_index<"orgcodes"_n, orgcode,
      eosio::indexed_by<"orgcodeidx"_n, eosio::const_mem_fun<orgcode, uint64_t, &orgcode::by_org_code>>
    > orgcode_index;

      // scoped by contract
    TABLE auth {
        name action;
        vector<name> authorized_contracts;
        uint64_t primary_key() const { return action.value; }
    };
    typedef eosio::multi_index<"auth"_n, auth> auth_table;

    void check_internal_auth (name action, string failure_identifier) {
        auth_table _auth(name(AUTHORITY_CONTRACT), _self.value);
        auto itr = _auth.find(action.value);
        check(itr != _auth.end(), failure_identifier + "no entry in authority table for this action and contract");
        auto authorized_contracts = itr->authorized_contracts;
        for(auto i = 0 ; i < authorized_contracts.size(); i++ ) {
            if(has_auth(authorized_contracts[i])) {
                return;
            }
        }
        check(false, failure_identifier + "Calling contract not in authorized list of accounts for action " + action.to_string());
    }

    void update_rank(name account, uint64_t badge_agg_seq_id, uint64_t old_balance, uint64_t new_balance) {

        ranks_table _ranks(get_self(), badge_agg_seq_id); // Use badge_agg_seq_id as scope
        statssetting_table _statssetting(get_self(), get_self().value);
        auto statssetting_itr = _statssetting.find(badge_agg_seq_id);
        check(statssetting_itr != _statssetting.end(), "no record in statssetting table");
        
        uint64_t current_no_of_ranks = statssetting_itr->current_no_of_ranks;
        uint64_t max_no_of_ranks = statssetting_itr->max_no_of_ranks;

        bool new_account = false;

        // Check and remove the player's name from the old score, if provided and different

        if(_ranks.begin() == _ranks.end() || new_balance > _ranks.begin()->balance || max_no_of_ranks > current_no_of_ranks) {

            if (old_balance != new_balance) {
                auto old_itr = _ranks.find(old_balance);
                if (old_itr != _ranks.end()) {
                    auto old_names = old_itr->accounts;
                    auto old_size = old_names.size();
                    old_names.erase(std::remove(old_names.begin(), old_names.end(), account), old_names.end());
                    auto new_old_size = old_names.size();
                    if(old_size == new_old_size) {
                        new_account = true;
                    }
                    if (old_names.empty()) {
                        _ranks.erase(old_itr); // Remove the score entry if no names are left
                    } else {
                        _ranks.modify(old_itr, get_self(), [&](auto& entry) {
                            entry.accounts = old_names;
                        });
                    }
                } else {
                    new_account = true;
                }
            }
            auto new_itr = _ranks.find(new_balance);
            if (new_itr == _ranks.end()) {
                _ranks.emplace(get_self(), [&](auto& entry) {
                    entry.balance = new_balance;
                    entry.accounts.push_back(account);
                });
            } else {
                if (std::find(new_itr->accounts.begin(), new_itr->accounts.end(), account) == new_itr->accounts.end()) {
                    _ranks.modify(new_itr, get_self(), [&](auto& entry) {
                        entry.accounts.push_back(account);
                    });
                } // If player name already exists for this score, no action needed
            }
            if(new_account && max_no_of_ranks == current_no_of_ranks) {
                auto rank_itr = _ranks.begin();
                auto lowest_rank_names = rank_itr->accounts;
                lowest_rank_names.pop_back();
                if (lowest_rank_names.empty()) {
                    _ranks.erase(rank_itr); 
                } else {
                    _ranks.modify(rank_itr, get_self(), [&](auto& entry) {
                        entry.accounts = lowest_rank_names;
                    });
                }
            } else if (new_account && max_no_of_ranks > current_no_of_ranks) {
                _statssetting.modify(statssetting_itr, get_self(), [&](auto& entry) {
                    entry.current_no_of_ranks++;
                });
            }
        }
        

    }

    // Function to fetch the new balance from the achievements table, scoped by account.
    uint64_t get_new_balance(name account, uint64_t badge_agg_seq_id) {
        // Access the achievements table with the account as the scope.
        achievements_table achievements(name(BOUNDED_AGG_CONTRACT), account.value);
        auto achv_itr = achievements.find(badge_agg_seq_id);
        
        // Check if the record exists and return the balance.
        if (achv_itr != achievements.end()) {
            return achv_itr->count; // Assuming 'balance' is the field storing the balance.
        } else {
            // Handle the case where there is no such achievement record.
            eosio::check(false, "Achievement record not found for the given badge_agg_seq_id.");
            return 0; // This return is formal as the check above will halt execution if triggered.
        }
    }
    
    checksum256 hash_active_status(const symbol& badge_symbol, const name& badge_status, const name& seq_status) {
        // Create a data string from badge_symbol, badge_status, and seq_status
        string data_str = badge_symbol.code().to_string() + badge_status.to_string() + seq_status.to_string();

        // Return the sha256 hash of the concatenated string
        return sha256(data_str.data(), data_str.size());
    }

};
