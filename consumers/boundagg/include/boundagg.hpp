#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <string>
#include <eosio/crypto.hpp>
#include <eosio/system.hpp>

using namespace eosio;
using namespace std;

#define AUTHORITY_CONTRACT "autgr"
#define ORG_CONTRACT "autgr"
#define ORCHESTRATOR_CONTRACT "orchestrator"
#define NEW_BADGE_ISSUANCE_NOTIFICATION ORCHESTRATOR_CONTRACT"::notifyachiev"

CONTRACT boundagg : public contract {
public:
    using contract::contract;

    [[eosio::on_notify(NEW_BADGE_ISSUANCE_NOTIFICATION)]] void notifyachiev(asset badge_asset, 
    name from, 
    name to, 
    string memo, 
    vector<name> notify_accounts);

    ACTION activeseq(symbol agg_symbol, vector<uint64_t> seq_ids);
    ACTION activeseqli(symbol agg_symbol);
    ACTION activeseqlp(symbol agg_symbol);
    ACTION initagg(symbol agg_symbol, string description);

    ACTION initseq(symbol agg_symbol, string description);
    ACTION pauseseq(symbol agg_symbol, vector<uint64_t> seq_ids);
    ACTION endseq(symbol agg_symbol, vector<uint64_t> seq_ids);

    ACTION addbadgeli(symbol agg_symbol, vector<symbol> badge_symbols);
    ACTION addbadge(symbol agg_symbol, vector<uint64_t> seq_ids, vector<symbol> badge_symbols);
    
private:

    struct [[eosio::table]] agg {
        uint64_t seq_id;
        name seq_status; // init, end, active, pause
        string description;
        time_point_sec active_time;
        time_point_sec end_time;
        time_point_sec init_time;
        time_point_sec pause_time;

        uint64_t primary_key() const { return seq_id; }
    };

    struct [[eosio::table]] agginfo {
        symbol agg_symbol;
        uint64_t last_init_seq_id;
        uint64_t last_end_seq_id;
        uint64_t last_active_seq_id;
        uint64_t last_pause_seq_id;

        uint64_t primary_key() const { return agg_symbol.raw(); }
    };

    struct [[eosio::table]] badgeaggseq {
        uint64_t badge_agg_seq_id; // Primary key: Unique ID for each badge-sequence association
        symbol agg_symbol;         // The aggregation symbol associated with this badge
        uint64_t seq_id;           // The sequence ID this badge is associated with
        symbol badge_symbol;       // The symbol representing the badge
        name badge_status;         // The status of the badge (e.g., "active")
        name seq_status;           // The status of the sequence copied from the agg table

        uint64_t primary_key() const { return badge_agg_seq_id; }
        uint128_t by_agg_seq() const { return combine_keys(agg_symbol.raw(), seq_id); }
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

    struct [[eosio::table]] achievements {
        uint64_t badge_agg_seq_id;
        uint64_t count;

        uint64_t primary_key() const { return badge_agg_seq_id; }
    };


    struct [[eosio::table]] orgcode {
        name org;
        name org_code;

        uint64_t primary_key() const { return org.value; }
    };

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

    typedef eosio::multi_index<"aggs"_n, agg> agg_table;
    typedef eosio::multi_index<"agginfos"_n, agginfo> agginfo_table;
    typedef eosio::multi_index<"badgeaggseqs"_n, badgeaggseq,
        eosio::indexed_by<"byaggseq"_n, eosio::const_mem_fun<badgeaggseq, uint128_t, &badgeaggseq::by_agg_seq>>,
        eosio::indexed_by<"bybadgestat"_n, eosio::const_mem_fun<badgeaggseq, checksum256, &badgeaggseq::by_badge_status>>
    > badgeaggseq_table;
    typedef eosio::multi_index<"orgcodes"_n, orgcode> orgcode_table;
    typedef eosio::multi_index<"achievements"_n, achievements> achievements_table;

    checksum256 hash_active_status(const symbol& badge_symbol, const name& badge_status, const name& seq_status) {
        // Create a data string from badge_symbol, badge_status, and seq_status
        string data_str = badge_symbol.code().to_string() + ":" + badge_status.to_string() + ":" + seq_status.to_string();

        // Return the sha256 hash of the concatenated string
        return sha256(data_str.data(), data_str.size());
    }

    name get_org_from_agg_symbol(const symbol& agg_symbol, string failure_identifier) {
        // Simplified logic to match agg_symbol to org
        // This function needs actual implementation details based on your data model
        orgcode_table orgcodes(name(ORG_CONTRACT), name(ORG_CONTRACT).value);
        auto itr = orgcodes.begin(); // Example: getting the first entry for demonstration
        check(itr != orgcodes.end(), failure_identifier + "Organization not found for agg_symbol");
        return itr->org;
    }

    name get_seq_status(const symbol& agg_symbol, const uint64_t& seq_id, string failure_identifier) {
        agg_table aggs(get_self(), agg_symbol.code().raw());
        auto agg_itr = aggs.find(seq_id);
        check(agg_itr != aggs.end(), failure_identifier + "Sequence ID not found in agg table");
        return agg_itr->seq_status;
    }

    void add_badges_to_aggseq(symbol agg_symbol, uint64_t seq_id, vector<symbol> badge_symbols, name seq_status, string failure_identifier) {
        badgeaggseq_table badgeaggseqs(get_self(), agg_symbol.code().raw());
        for (auto badge_symbol : badge_symbols) {
            badgeaggseqs.emplace(get_self(), [&](auto& b) {
                b.badge_agg_seq_id = badgeaggseqs.available_primary_key();
                b.agg_symbol = agg_symbol;
                b.seq_id = seq_id;
                b.badge_symbol = badge_symbol;
                b.badge_status = "active"_n;
                b.seq_status = seq_status;
            });
        }
    }

    // New function to get the last initialized sequence ID
    uint64_t get_last_init_seq_id(symbol agg_symbol, string failure_identifier) {
        agginfo_table agginfos(get_self(), get_self().value);
        auto agginfo_itr = agginfos.find(agg_symbol.code().raw());
        check(agginfo_itr != agginfos.end(), failure_identifier + "Aggregation information not found");

        return agginfo_itr->last_init_seq_id;
    }

    // New function to get the last paused sequence ID
    uint64_t get_last_paused_seq_id(symbol agg_symbol, string failure_identifier) {
        agginfo_table agginfos(get_self(), get_self().value);
        auto agginfo_itr = agginfos.find(agg_symbol.code().raw());
        check(agginfo_itr != agginfos.end(), failure_identifier + "Aggregation information not found");

        return agginfo_itr->last_pause_seq_id;
    }

    void activate_seq_status(symbol agg_symbol, uint64_t seq_id, string failure_identifier) {
        // Update agg table
        agg_table aggs(get_self(), agg_symbol.code().raw());
        auto itr = aggs.find(seq_id);
        check(itr != aggs.end(), failure_identifier + "Sequence ID not found");
        check(itr->seq_status == "init"_n || itr->seq_status == "pause"_n, failure_identifier + "Sequence must be in 'init' or 'pause' status to activate");
        aggs.modify(itr, get_self(), [&](auto& item) {
            item.seq_status = "active"_n;
            item.active_time = time_point_sec(current_time_point());
        });

        // Update badgeaggseq table for corresponding seq_id
        badgeaggseq_table badgeaggseqs(get_self(), agg_symbol.code().raw());
        auto badge_idx = badgeaggseqs.get_index<"byaggseq"_n>();
        for (auto badge_itr = badge_idx.lower_bound(seq_id); badge_itr != badge_idx.end() && badge_itr->seq_id == seq_id; ++badge_itr) {
            badgeaggseqs.modify(*badge_itr, get_self(), [&](auto& badge) {
                badge.seq_status = "active"_n;
            });
        }
    }

};
