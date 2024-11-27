#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <hyperloglog.hpp>
#include <string>
#include <vector>

using namespace eosio;
using namespace std;

#define ORCHESTRATOR_CONTRACT "orchyyyyyyyy"
#define BOUNDED_AGG_CONTRACT "baggyyyyyyyy"

#define NEW_BADGE_ISSUANCE_NOTIFICATION ORCHESTRATOR_CONTRACT"::notifyachiev"
#define SERIES_START_NOTIFICATION BOUNDED_AGG_CONTRACT"::actseq"
#define SERIES_END_NOTIFICATION BOUNDED_AGG_CONTRACT"::endseq"

CONTRACT subscription : public contract {
  public:
    using contract::contract;
    
    [[eosio::on_notify(SERIES_START_NOTIFICATION)]]
    void startseries(name org, symbol agg_symbol, vector<uint64_t> seq_ids);

    [[eosio::on_notify(SERIES_END_NOTIFICATION)]]    
    void endseries(name org, symbol agg_symbol, vector<uint64_t> seq_ids);

    ACTION cleanup(name org, uint64_t new_seq_id, uint64_t curr_seq_id);

    ACTION cleanupram();

    /**
    * Handles service purchases based on memo details.
    * 
    * - Parses the memo to determine if it is for membership, series, or a package.
    * - Processes the payment accordingly by updating the due amount or adding a new package.
    * 
    * @param from Sender of the payment.
    * @param to Recipient (contract account).
    * @param amount Amount paid.
    * @param memo Memo describing the purchase (format: <org>:<type>:<package>).
    */

    [[eosio::on_notify("*::transfer")]]
    void buyservice(name from, name to, asset amount, string memo);

    /**
    * Handles package purchases paid using WRAM tokens.
    * 
    * - Parses the memo to extract organization and package information.
    * - Verifies the package status and processes the purchase.
    * - Returns any unused balance to the sender.
    * 
    * @param from Sender of the payment.
    * @param to Recipient (contract account).
    * @param amount Amount paid.
    * @param memo Memo describing the purchase (format: <ORG>:PACK:<package>).
    */


    [[eosio::on_notify("eosio.wram::transfer")]]
    void buypackwram(name from, name to, asset amount, string memo);

    /**
    * Notifies the achievement or badge issuance.
    * 
    * - Checks the service status of the organization in both members and series tables.
    * - If the organization does not exist, inserts it with default perks.
    * - Uses HyperLogLog to track unique member counts.
    * - If no active package is found, it creates one.
    * 
    * @param org Organization name.
    * @param badge_asset Asset representing the badge.
    * @param from Issuer of the badge.
    * @param to Recipient of the badge.
    * @param memo Description or context of the badge.
    * @param notify_accounts Accounts to notify.
    */

    [[eosio::on_notify(NEW_BADGE_ISSUANCE_NOTIFICATION)]]
    void notifyachiev(
        name org,
        asset badge_asset,
        name from,
        name to,
        string memo,
        vector<name> notify_accounts
    );

    /**
    * Creates a new package entry in the packages table.
    * 
    * - Adds a new package if it does not already exist.
    * - The package can be configured with issuance size, expiration, and other attributes.
    * 
    * @param authorized Account authorized to create the package.
    * @param package Package name.
    * @param unique_issuance_size Number of unique issuances.
    * @param expiry_duration_in_secs Expiry duration in seconds.
    * @param ram_in_bytes RAM size in bytes.
    * @param only_ram_as_payment_method Boolean to indicate if RAM tokens are used.
    * @param active Boolean indicating if the package is active.
    */

    ACTION newpack(
        name package,
        uint64_t unique_issuance_size,
        uint64_t expiry_duration_in_secs,
        asset ram_in_bytes,
        bool only_ram_as_payment_method,
        bool active
    );


    /**
    * Disables a package by setting its active status to false.
    * 
    * @param package Package name to disable.
    */
    ACTION disablepack(name package);

    /**
    * Enables a previously disabled package by setting its active status to true.
    * 
    * @param package Package name to enable.
    */
    ACTION enablepack(name package);

    /**
    * Sets the volatility percentage for the packages.
    * 
    * - The volatility percentage is stored in the volatility table.
    * 
    * @param vt_percent Volatility percentage (e.g., 2534 for 25.34%).
    */
    ACTION setvt(uint64_t vt_percent);

    /**
    * Sets the RAM rate
    * 
    * - The rate is stored in the ramcost table.
    * 
    * @param per_byte_cost Cost per byte.
    */
    ACTION setramrate(extended_asset per_byte_cost);

    /**
    * Initializes the first billing cycle.
    * 
    * - Ensures that the start time is before the end time.
    * - Creates a new entry in the billcycle table.
    * 
    * @param start_time Start time of the cycle.
    * @param end_time End time of the cycle.
    */
    ACTION firstcycle(time_point_sec start_time, time_point_sec end_time);

    /**
    * Creates the next billing cycle.
    * 
    * - Ensures continuity between previous and new cycles.
    * - Updates the sequence and adds a new cycle entry.
    * 
    * @param end_time End time of the new cycle.
    */
    ACTION nextcycle(time_point_sec end_time);

    /**
    * Sets the grace period for dues.
    * 
    * - Stores the grace period in the duetime singleton table.
    * 
    * @param due_grace_seconds Grace period in seconds.
    */
    ACTION setduetime(uint64_t due_grace_seconds);

    /**
    * Generates a bill for the members.
    * 
    * - Checks the current billing cycle and calculates due amounts.
    * - If within the free member limit, no due amount is generated.
    * 
    * @param org Organization name.
    */
    ACTION memberbill(name org);

    /**
    * Checks the service status of a member organization.
    * 
    * - Verifies if the due amount is cleared within the grace period.
    * - Updates the service status accordingly.
    * 
    * @param org Organization name.
    */
    ACTION chkmemstatus(name org);

    /**
    * Updates the service status of a member organization.
    * 
    * @param org Organization name.
    * @param service_active Boolean indicating if the service is active.
    */
    ACTION setmemstatus(name org, bool service_active);

    /**
    * Sets the cost for additional members.
    * 
    * - Updates the membercost table with the provided cost.
    * 
    * @param first_additional_member_admin_cost Cost for additional members.
    */
    ACTION setmemcost(extended_asset first_additional_member_admin_cost);


    /**
    * Generates a bill for series subscriptions.
    * 
    * - Checks the current cycle and calculates the due amount.
    * - If within the free series limit, no due amount is generated.
    * 
    * @param org Organization name.
    */
    ACTION seriesbill(name org);

    /**
    * Checks the service status of a series subscription.
    * 
    * - Verifies if dues are paid within the grace period.
    * - Updates the service status accordingly.
    * 
    * @param org Organization name.
    */
    ACTION chkserstatus(name org);

    /**
    * Updates the service status of a series subscription.
    * 
    * @param org Organization name.
    * @param service_active Boolean indicating if the service is active.
    */
    ACTION setserstatus(name org, bool service_active);

    /**
    * Sets the cost for additional series subscriptions.
    * 
    * - Updates the seriescost table with the provided cost.
    * 
    * @param additonal_series_admin_cost Cost for additional series.
    */
    ACTION setsercost(extended_asset additonal_series_admin_cost);


    ACTION freemembers (uint64_t free_members);

    ACTION freememcycle (uint64_t free_cycles);

    ACTION freeseries (uint64_t free_series);

    ACTION freesercycle (uint64_t free_cycles);

  private:

    // Table definition for sequences.
    TABLE sequences {
        name key;
        uint64_t last_used_value;

        uint64_t primary_key() const { return key.value; }
    };
    typedef eosio::multi_index<"sequences"_n, sequences> sequences_table;

    TABLE packages {
        name package;
        string descriptive_name;
        uint64_t action_size;
        uint64_t expiry_duration_in_secs;
        uint64_t max_active_series;
        asset cost;
        bool active;

        uint64_t primary_key const {return package.value ;}
    };
    typedef eosio::multi_index<"packages"_n, packages> packages_table;    
    // Table definition for packages.
    TABLE packages {
        name package;
        uint64_t unique_issuance_size;
        uint64_t expiry_duration_in_secs;
        asset ram_in_bytes;
        bool only_ram_as_payment_method;
        bool active;

        uint64_t primary_key() const { return package.value; }
    };
    typedef eosio::multi_index<"packages"_n, packages> packages_table;

    // Table definition for volatility.
    TABLE volatility {
        uint64_t seq_id;
        uint64_t value; // 2 decimals.
        time_point_sec last_updated;

        uint64_t primary_key() const { return seq_id; }
    };
    typedef eosio::multi_index<"volatility"_n, volatility> volatility_table;

    // Table definition for ramcost.
    TABLE ramcost {
        uint64_t seq_id;
        extended_asset per_byte_cost;
        time_point_sec last_updated;

        uint64_t primary_key() const { return per_byte_cost.quantity.symbol.code().raw(); }
    };
    typedef eosio::multi_index<"ramcost"_n, ramcost> ramcost_table;

    // Table definition for newpackage.
    TABLE newpackage {
        uint64_t seq_id;
        name package_name;
        uint64_t unique_issuance_size;
        uint64_t expiry_duration_in_secs;

        uint64_t primary_key() const { return seq_id; }
    };
    typedef eosio::multi_index<"newpackage"_n, newpackage> newpackage_table;

    // Table definition for currpackage.
    TABLE currpackage {
        uint64_t seq_id;
        uint64_t total_bought;
        vector<uint8_t> unique_issuance_hll;
        uint64_t total_used;
        time_point_sec expiry_time;

        uint64_t primary_key() const { return seq_id; }
    };
    typedef eosio::multi_index<"currpackage"_n, currpackage> currpackage_table;

    // Table definition for usedpackages.
    TABLE usedpackages {
        uint64_t seq_id;
        uint64_t total_bought;
        uint64_t total_used;
        time_point_sec expiry_time;

        uint64_t primary_key() const { return seq_id; }
    };
    typedef eosio::multi_index<"usedpackages"_n, usedpackages> usedpackages_table;

    // Billcycle table definition
    TABLE billcycle {
        uint64_t seq_id;
        time_point_sec start_time;
        time_point_sec end_time;

        uint64_t primary_key() const { return seq_id; }
    };
    typedef eosio::multi_index<"billcycle"_n, billcycle> billcycle_table;

    // Oldbillcycle table definition
    TABLE oldbillcycle {
        uint64_t seq_id;
        time_point_sec start_time;
        time_point_sec end_time;

        uint64_t primary_key() const { return seq_id; }
    };
    typedef eosio::multi_index<"oldbillcycle"_n, oldbillcycle> oldbillcycle_table;

        // Singleton table definition for duetime.
    TABLE duetime {
        uint64_t due_grace_seconds;

        uint64_t primary_key() const { return 0; }
    };
    typedef eosio::singleton<"duetime"_n, duetime> duetime_singleton;

    // Members table definition
    TABLE members {
        name org;
        uint64_t free_members;
        uint64_t free_cycles;
        vector<uint8_t> member_count_hll;
        uint64_t member_count;
        time_point_sec first_subscription_time;
        time_point_sec last_billed_amount_calculation_time;
        extended_asset total_due_amount;
        bool service_active;

        uint64_t primary_key() const { return org.value; }
    };
    typedef eosio::multi_index<"members"_n, members> members_table;



    // membercost table definition
    TABLE membercost {
        extended_asset first_additional_member_admin_cost;
        uint64_t primary_key() const { 
            return first_additional_member_admin_cost.quantity.symbol.code().raw();
        }
    };
    typedef eosio::multi_index<"membercost"_n, membercost> membercost_table;

    TABLE perks {
        name perkname;
        uint64_t value;

        uint64_t primary_key() const {return perkname.value;}
    };
    typedef eosio::multi_index<"perks"_n, perks> perks_table;

    TABLE series {
        name org;
        uint64_t free_series;
        uint64_t free_cycles;
        uint64_t highest_no_of_active_series_in_current_cycle;
        uint64_t current_active_series;
        time_point_sec first_subscription_time;
        time_point_sec last_billed_amount_calculation_time;
        extended_asset total_due_amount;
        bool service_active;

        uint64_t primary_key() const { return org.value; }
    };
    typedef eosio::multi_index<"series"_n, series> series_table;


    TABLE seriescost {
        extended_asset additonal_series_admin_cost;
        uint64_t primary_key() const { 
            return additonal_series_admin_cost.quantity.symbol.code().raw();
        }
    };
    typedef eosio::multi_index<"seriescost"_n, seriescost> seriescost_table;

    // Helper function to split memo strings.
    std::vector<std::string> split_pack_memo(const std::string& memo) {
        std::vector<std::string> result;
        std::string delimiter = ":";
        size_t start = 0;
        size_t end = memo.find(delimiter);
        while (end != std::string::npos) {
            result.push_back(memo.substr(start, end - start));
            start = end + delimiter.length();
            end = memo.find(delimiter, start);
        }
        result.push_back(memo.substr(start));
        return result;
    }

    // Helper function to convert RAM cost to native token.
    asset convert_ram_to_native(asset ram_cost, extended_asset fixed_per_byte_cost) {
        int64_t total_cost = (ram_cost.amount * fixed_per_byte_cost.quantity.amount);
        return asset(total_cost, fixed_per_byte_cost.quantity.symbol);
    }



   void move_to_oldbillcycle(const billcycle& cycle) {
        oldbillcycle_table old_cycles(get_self(), get_self().value);
        old_cycles.emplace(get_self(), [&](auto& old_cycle) {
            old_cycle.seq_id = cycle.seq_id;
            old_cycle.start_time = cycle.start_time;
            old_cycle.end_time = cycle.end_time;
        });

        billcycle_table bill_cycles(get_self(), get_self().value);
        auto itr = bill_cycles.find(cycle.seq_id);
        check(itr != bill_cycles.end(), "Cycle to be moved not found.");
        bill_cycles.erase(itr);

        print("Moved cycle ", cycle.seq_id, " to oldbillcycle.");
    }

    // Helper function to update member billing
    void update_member_billing(members_table::const_iterator member_itr, asset total_due, time_point_sec current_time, uint64_t updated_free_cycles) {
        members_table members(get_self(), get_self().value);
        members.modify(member_itr, get_self(), [&](auto& member) {
            member.total_due_amount.quantity += total_due;
            member.last_billed_amount_calculation_time = current_time;
            member.free_cycles = updated_free_cycles;
        });
    }

    // Helper function to update series billing
    void update_series_billing(series_table::const_iterator series_itr, asset total_due, time_point_sec current_time, uint8_t updated_free_cycles) {
        series_table series(get_self(), get_self().value);
        series.modify(series_itr, get_self(), [&](auto& s) {
            s.total_due_amount.quantity += total_due;
            s.last_billed_amount_calculation_time = current_time;
            s.free_cycles = updated_free_cycles;
        });
    }

    void insert_into_package(name org, name package_name, bool only_ram_as_payment_method) {
        // Load the packages table and find the package
        packages_table pkg_tbl(get_self(), get_self().value);
        auto pkg_itr = pkg_tbl.find(package_name.value);
        check(pkg_itr != pkg_tbl.end(), "Package not found.");

        // Ensure the package is active
        check(pkg_itr->active, "Package is not active.");

        // Ensure the only_ram_as_payment_method matches
        check(pkg_itr->only_ram_as_payment_method == only_ram_as_payment_method,
              "payment method mismatch.");


    }

    void update_member_due_amount(name org, asset paid_amount, name from) {
        check(paid_amount.amount > 0, "Paid amount must be positive.");

        members_table members(get_self(), get_self().value);
        auto member_itr = members.find(org.value);
        check(member_itr != members.end(), "Organization not found in members table.");

        // Ensure the contract of the total_due_amount matches the first receiver
        check(get_first_receiver() == member_itr->total_due_amount.contract, "Asset contract does not match the first receiver.");

        // Ensure the asset symbols match
        check(member_itr->total_due_amount.quantity.symbol == paid_amount.symbol, 
            "Asset symbol mismatch between total due amount and paid amount.");

        int64_t new_due_amount = member_itr->total_due_amount.quantity.amount - paid_amount.amount;

        if (new_due_amount < 0) {
            asset surplus(paid_amount.amount - member_itr->total_due_amount.quantity.amount, paid_amount.symbol);

            members.modify(member_itr, get_self(), [&](auto& member) {
                member.total_due_amount.quantity = asset(0, paid_amount.symbol);
                member.service_active = true;
            });

            action(
                permission_level{get_self(), "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(get_self(), from, surplus, std::string("Surplus refund"))
            ).send();

            print("Surplus of ", surplus, " returned to ", from);
        } else {
            members.modify(member_itr, get_self(), [&](auto& member) {
                member.total_due_amount.quantity = asset(new_due_amount, paid_amount.symbol);
                member.service_active = (new_due_amount == 0);
            });

            print("Total due amount for org ", org, " updated to ", new_due_amount);
        }
    }

    void update_series_due_amount(name org, asset paid_amount, name from) {
        
        check(paid_amount.amount > 0, "Paid amount must be positive.");

        series_table series_tbl(get_self(), get_self().value);
        auto series_itr = series_tbl.find(org.value);
        check(series_itr != series_tbl.end(), "Organization not found in series table.");

        // Ensure the contract of the total_due_amount matches the first receiver
        check(get_first_receiver() == series_itr->total_due_amount.contract, "Asset contract does not match the first receiver.");

        // Ensure the asset symbols match
        check(series_itr->total_due_amount.quantity.symbol == paid_amount.symbol, 
            "Asset symbol mismatch between total due amount and paid amount.");

        int64_t new_due_amount = series_itr->total_due_amount.quantity.amount - paid_amount.amount;

        if (new_due_amount < 0) {
            asset surplus(paid_amount.amount - series_itr->total_due_amount.quantity.amount, paid_amount.symbol);

            series_tbl.modify(series_itr, get_self(), [&](auto& s) {
                s.total_due_amount.quantity = asset(0, paid_amount.symbol);
                s.service_active = true;
            });

            action(
                permission_level{get_self(), "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(get_self(), from, surplus, std::string("Surplus refund"))
            ).send();

            print("Surplus of ", surplus, " returned to ", from);
        } else {
            series_tbl.modify(series_itr, get_self(), [&](auto& s) {
                s.total_due_amount.quantity = asset(new_due_amount, paid_amount.symbol);
                s.service_active = (new_due_amount == 0);
            });

            print("Total due amount for org ", org, " updated to ", new_due_amount);
        }
    }

    void buy_new_package(name org, name package_name, asset amount, name from) {
        packages_table packages(get_self(), get_self().value);
        auto package_itr = packages.find(package_name.value);
        check(package_itr != packages.end(), "Package not found");
        check(package_itr->active, "Package is not active");
        check(!package_itr->only_ram_as_payment_method, "This pack can only be bought via RAM tokens");

        asset base_cost = package_itr->ram_in_bytes;

        volatility_table volatility(get_self(), get_self().value);
        auto vol_itr = volatility.end();
        check(vol_itr != volatility.begin(), "No volatility data found");
        --vol_itr;

        double volatility_percent = vol_itr->value / 10000.0;
        asset final_cost_in_ram = base_cost + base_cost * volatility_percent;

        ramcost_table ramcost(get_self(), get_self().value);
        auto ramrate_itr = ramcost.end();
        check(ramrate_itr != ramcost.begin(), "No RAM rate data found");
        --ramrate_itr;

        // Ensure the contract of the asset matches the first receiver
        check(get_first_receiver() == ramrate_itr->per_byte_cost.contract, "Asset contract does not match the first receiver.");
        // Ensure the asset symbols match
        check(ramrate_itr->per_byte_cost.quantity.symbol == amount.symbol, 
            "Asset symbol mismatch between total due amount and paid amount.");
        asset native_cost = convert_ram_to_native(final_cost_in_ram, ramrate_itr->per_byte_cost);

        check(amount >= native_cost, "Insufficient funds to purchase the package");

        asset surplus = amount - native_cost;
        if (surplus.amount > 0) {
            action(
                permission_level{get_self(), "active"_n},
                "eosio.token"_n,
                "transfer"_n,
                std::make_tuple(get_self(), from, surplus, std::string("Returning surplus balance"))
            ).send();
        }

        uint64_t seq_id = get_next_sequence("newpackage"_n);

        newpackage_table new_pkg_tbl(get_self(), org.value);
        new_pkg_tbl.emplace(get_self(), [&](auto& new_pkg) {
            new_pkg.seq_id = seq_id;
            new_pkg.package_name = package_name;
            new_pkg.unique_issuance_size = package_itr->unique_issuance_size;
            new_pkg.expiry_duration_in_secs = package_itr->expiry_duration_in_secs;
        });

        print("New package inserted: ", package_name, " with seq_id = ", seq_id);
    }


    // Helper function to get the last sequence value for a given key
    uint64_t get_last_sequence(name key) {
        sequences_table seq_tbl(get_self(), get_self().value);
        auto seq_itr = seq_tbl.find(key.value);
        check(seq_itr != seq_tbl.end(), "Sequence not found for the provided key.");
        return seq_itr->last_used_value;
    }

    // Helper function to get the next sequence ID
    uint64_t get_next_sequence(name key) {
        sequences_table seq_tbl(get_self(), get_self().value);
        auto seq_itr = seq_tbl.find(key.value);

        if (seq_itr == seq_tbl.end()) {
            // If sequence doesn't exist, initialize it with 1
            seq_tbl.emplace(get_self(), [&](auto& seq) {
                seq.key = key;
                seq.last_used_value = 1;
            });
            return 1;
        } else {
            // Increment the sequence ID
            seq_tbl.modify(seq_itr, get_self(), [&](auto& seq) {
                seq.last_used_value++;
            });
            return seq_itr->last_used_value;
        }
    }


    void check_series_payment_due (name org) {
        series_table series(get_self(), get_self().value);
        auto series_itr = series.find(org.value);
        if(series_itr != series.end()) {
            check(series_itr->service_active == true, "Check if org need to pay series due amount");
        }
    }

    void init_series_usage(name org) {
        uint64_t free_series = get_perk_value(name("freeseries"));
        uint64_t free_cycles = get_perk_value(name("freesercycle"));

        series_table series(get_self(), get_self().value);
        auto series_itr = series.find(org.value);
        if(series_itr == series.end()) {
            series.emplace(get_self(), [&](auto& s) {
                s.org = org;
                s.free_series = free_series;
                s.free_cycles = free_cycles;
                s.first_subscription_time = eosio::current_time_point();
                s.service_active = true;
            });
        } else {
            series.modify(series_itr, get_self(), [&](auto& s) {
                s.first_subscription_time = eosio::current_time_point();
                s.service_active = true;
            });
        }
    }

    void init_membership_usage(members_table& members, name org, name to) {
        uint8_t b = 7;
        uint32_t m = 1 << b;
        vector<uint8_t> M(m, 0);
        hll::HyperLogLog hll(b, m, M);
        hll.add(to.to_string().c_str(), to.to_string().size());
        uint64_t free_members = get_perk_value(name("freemembers"));
        uint64_t free_cycles = get_perk_value(name("freememcycle"));
        members.emplace(get_self(), [&](auto& row) {
            row.org = org;
            row.free_members = free_members;
            row.free_cycles = free_cycles;
            row.first_subscription_time = eosio::current_time_point();
            row.member_count_hll = hll.registers();
            row.member_count = 1;
            row.service_active = true;
        });
    }

    void update_package_usage(name org, asset badge_asset, name to) {
        uint8_t b = 7;
        uint32_t m = 1 << b;
        std::string hll_str = badge_asset.symbol.code().to_string() + to.to_string();
        currpackage_table currpackage(get_self(), org.value);
        auto currpackage_itr = currpackage.begin();
        if(currpackage_itr == currpackage.end()) {
            // first time usage.
            newpackage_table newpackage(get_self(), org.value);

            // Ensure the newpackage table is not empty
            auto newpackage_itr = newpackage.begin();
            check(newpackage_itr != newpackage.end(), "First time usage. No records found in the newpackage table.");
            
            vector<uint8_t> M(m, 0);
            hll::HyperLogLog hll(b, m, M);
            hll.add(hll_str.c_str(), hll_str.size());
            // Insert the first record from newpackage into currpackage
            currpackage.emplace(get_self(), [&](auto& row) {
                row.seq_id = newpackage_itr->seq_id;
                row.total_bought = newpackage_itr->unique_issuance_size;
                row.total_used = 1;  // Set total_used to 0 initially
                row.unique_issuance_hll = hll.registers();
                // Calculate expiry_time as current time + expiry_duration (in seconds)
                row.expiry_time = time_point_sec(current_time_point().sec_since_epoch() + newpackage_itr->expiry_duration_in_secs);
            });

            newpackage.erase(newpackage_itr);
        } else if(currpackage_itr->total_bought <= currpackage_itr->total_used || currpackage_itr->expiry_time < current_time_point()) {
            
            vector<uint8_t> M = currpackage_itr->unique_issuance_hll;
            hll::HyperLogLog hll(b, m, M);
            double estimate_before = hll.estimate();
            hll.add(hll_str.c_str(), hll_str.size());
            double estimate_after = hll.estimate();
            if(estimate_before != estimate_after) {

                newpackage_table newpackage(get_self(), org.value);
                auto newpackage_itr = newpackage.begin();
                check(newpackage_itr != newpackage.end(), "New member-badge issuance. No records found in the newpackage table.");
                usedpackages_table used_pkg_tbl(get_self(), org.value);

                // Move the package from currpackage to usedpackages
                used_pkg_tbl.emplace(get_self(), [&](auto& used_pkg) {
                    used_pkg.seq_id = currpackage_itr->seq_id;
                    used_pkg.total_bought = currpackage_itr->total_bought;
                    used_pkg.total_used = currpackage_itr->total_used;
                    used_pkg.expiry_time = currpackage_itr->expiry_time;
                });

                currpackage.erase(currpackage_itr);

                currpackage.emplace(get_self(), [&](auto& row) {
                    row.seq_id = newpackage_itr->seq_id;
                    row.total_bought = newpackage_itr->unique_issuance_size;
                    row.total_used = 1;
                    // Set total_used to 0 initially
                    row.unique_issuance_hll = hll.registers();
                    // Calculate expiry_time as current time + expiry_duration (in seconds)
                    row.expiry_time = time_point_sec(current_time_point().sec_since_epoch() + newpackage_itr->expiry_duration_in_secs);
                });
                newpackage.erase(newpackage_itr);
            }

        } else {
            vector<uint8_t> M = currpackage_itr->unique_issuance_hll;
            hll::HyperLogLog hll(b, m, M);
            double estimate_before = hll.estimate();
            hll.add(hll_str.c_str(), hll_str.size());
            double estimate_after = hll.estimate();
            if (estimate_before != estimate_after) {
                currpackage.modify(currpackage_itr, get_self(), [&](auto& row) {
                    // Append or modify the unique_issuance_hll data
                    row.unique_issuance_hll = hll.registers();
                    row.total_used += 1;
                });
            }
        }
    }

    void insert_or_update_perk(name perkname, uint64_t value) {
        perks_table perks(get_self(), get_self().value);
        auto itr = perks.find(perkname.value);

        if (itr == perks.end()) {
            // Record doesn't exist, so create a new one
            perks.emplace(get_self(), [&](auto& row) {
                row.perkname = perkname;
                row.value = value;
            });
        } else {
            // Record exists, so modify the existing one
            perks.modify(itr, get_self(), [&](auto& row) {
                row.value = value;
            });
        }
    }

    uint64_t get_perk_value(name perkname) {
        perks_table perks(get_self(), get_self().value);
        auto itr = perks.find(perkname.value);

        if (itr == perks.end()) {
            // Return 0 or any default value if the perk is not found
            print("Perk ", perkname.to_string(), " not found.");
            return 0;
        } else {
            return itr->value;
        }
    }
};
