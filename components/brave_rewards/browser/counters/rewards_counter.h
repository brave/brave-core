// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_REWARDS_DATA_COUNTERS_REWARDS_COUNTER_H_
#define BRAVE_BROWSER_REWARDS_DATA_COUNTERS_REWARDS_COUNTER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/browsing_data/core/counters/browsing_data_counter.h"

class Profile;

namespace rewards_counter {
  using OnDataCountedCallback =
      base::OnceCallback<void(int64_t, uint64_t)>;
}

// A RewardsDataCounter that counts the number of Rewards
// Auto-Contribute sites as seen on the brave://settings#clearRewardsData page.
// Derives from BrowsingDataCounter which all other data counters derive from.
// see Cr src -> history_counter, cache_counter, downloads_counter, etc
// for further impl info
class RewardsCounter : public browsing_data::BrowsingDataCounter {
 public:
  // Results returned for counter
  class RewardsResult : public FinishedResult {
   public:
    RewardsResult(const RewardsCounter* source,
        ResultInt site_count,
        base::string16 previous_reconcile_date);
    ~RewardsResult() override;

    // The date the last contribution was made
    base::string16 Date() const;

   private:
    base::string16 date_;
    DISALLOW_COPY_AND_ASSIGN(RewardsResult);
  };

  explicit RewardsCounter(Profile* profile);
  ~RewardsCounter() override;

  const char* GetPrefName() const override;

 private:
  void Count() override;
  void OnRewardsCounted(
      const ResultInt count,
      uint64_t previous_reconcile_date);
  Profile* profile_;
  base::WeakPtrFactory<RewardsCounter> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RewardsCounter);
};

#endif  // BRAVE_BROWSER_REWARDS_DATA_COUNTERS_REWARDS_COUNTER_H_
