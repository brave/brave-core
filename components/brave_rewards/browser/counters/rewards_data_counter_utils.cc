// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_rewards/browser/counters/rewards_data_counter_utils.h"  // NOLINT

#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_rewards/browser/counters/rewards_counter.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/text/bytes_formatting.h"

namespace rewards_data_counter_utils {

base::string16 GetBraveCounterTextFromResult(
    const browsing_data::BrowsingDataCounter::Result* result,
    Profile* profile) {
  std::string pref_name = result->source()->GetPrefName();
  base::string16 rewards_counter_text =
      GetAutoContributeCounterText(pref_name, result);
  if (!rewards_counter_text.empty()) {
    return rewards_counter_text;
  }

  return GetCounterTextFromResult(result);
}

base::string16 GetRewardsClearingText(
		const std::string& pref_name,
		bool in_progress) {
	if (pref_name == brave_rewards::prefs::kRewardsAutoContributeSites &&
			in_progress) {
		return l10n_util::GetStringUTF16(IDS_REWARDS_CONTRIBUTION_IN_PROGRESS);
	}
	return base::string16();
}

base::string16 GetAutoContributeCounterText(
    const std::string& pref_name,
    const browsing_data::BrowsingDataCounter::Result* result) {
  if (pref_name == brave_rewards::prefs::kRewardsAutoContributeSites) {
    const RewardsCounter::RewardsResult* rewards_result =
        static_cast<const RewardsCounter::RewardsResult*>(result);
    if (rewards_result->Value() == -1) { // contribution in progress
      return l10n_util::GetStringUTF16(IDS_REWARDS_CONTRIBUTION_IN_PROGRESS);
    } else if (rewards_result->Date().empty()) {
      // check for date also
      return l10n_util::GetPluralStringFUTF16(
          IDS_DEL_REWARDS_COUNTER, rewards_result->Value());
    } else {
       return l10n_util::GetPluralStringFUTF16(
          IDS_DEL_REWARDS_COUNTER, rewards_result->Value()) +
          l10n_util::GetStringFUTF16(
            IDS_DEL_REWARDS_COUNTER_LAST_DATE_TEXT,
            rewards_result->Date());
    }
  }
  return base::string16();
}

base::string16 GetCounterTextFromResult(
    const browsing_data::BrowsingDataCounter::Result* result) {
  base::string16 text;
  std::string pref_name = result->source()->GetPrefName();

  if (!result->Finished()) {
    // The counter is still counting.
    text = l10n_util::GetStringUTF16(IDS_CLEAR_BROWSING_DATA_CALCULATING);
  } else if (pref_name == brave_rewards::prefs::kRewardsAutoContributeSites) {
    // number of auto-contribute sites
    browsing_data::BrowsingDataCounter::ResultInt count =
        static_cast<const
          browsing_data::BrowsingDataCounter::FinishedResult*>(result)->Value();
    base::string16 date = static_cast<const
          RewardsCounter::RewardsResult*>(result)->Date();
    text = l10n_util::GetPluralStringFUTF16(
        IDS_DEL_REWARDS_COUNTER, count);
  }
  // we don't use a counter for "All Rewards Data"
  return text;
}

}  // namespace browsing_data_counter_utils
