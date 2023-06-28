#include <checks.hpp>

ACTION checks::ownnotown(name org, name account, vector<sbadges> own, vector<sbadges> not_own) {
  for(auto i = 0; i < own.size(); i++) {
    own_check(org, account, own[i].badge, own[i].count);
  }
  for(auto i = 0; i < not_own.size(); i++) {
    not_own_check(org, account, not_own[i].badge, not_own[i].count);
  }
}

ACTION checks::lookback(name org, name account, name series, uint8_t lookback, uint8_t musthave) {
  check(musthave <= lookback, "UNSUPPORTED INPUT: musthave cannot be greater than lookback");
  check(lookback > 0, "UNSUPPORTED INPUT: lookback should be atleast 1");
  if(musthave == 0) {
    return;
  }
  uint64_t last_seq_id = latest_seq_id(org, series);
  vector<uint64_t> sequences;
  sequences.push_back(last_seq_id);
  for (auto i = 0; i < lookback - 1; i++) {
    last_seq_id = last_seq_id - 1;
    if(last_seq_id > 0) {
      sequences.push_back(last_seq_id);
    }
  }
  uint8_t hascount = series_own_count (org, account, series, sequences);
  if ( hascount >= musthave) {
    return;
  } else {
    check(false, "CHECK FAILED: account has only " + to_string(hascount) + "; needed " + to_string(musthave));
  }

}


// todo call lookback action with musthave 1 and lookback 1
ACTION checks::haslatest(name org, name account, name series) {

  uint64_t last_seq_id = latest_seq_id(org, series);
  check(last_seq_id > 0, "CHECK FAILED: no badge defined in series");
  
  vector<uint64_t> sequences;
  sequences.push_back(last_seq_id);
  uint8_t hascount = series_own_count (org, account, series, sequences);
  if(hascount == 1) {
    return;
  } else {
    check(false, "CHECK FAILED: account does not have the latest in series");
  }

}