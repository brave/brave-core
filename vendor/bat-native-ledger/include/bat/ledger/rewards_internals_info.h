/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_REWARDS_INTERNALS_INFO_
#define BAT_LEDGER_REWARDS_INTERNALS_INFO_

namespace ledger {

struct CurrentReconcileInfo;

LEDGER_EXPORT struct RewardsInternalsInfo {
  RewardsInternalsInfo();
  ~RewardsInternalsInfo();
  RewardsInternalsInfo(const RewardsInternalsInfo& info);

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::string payment_id;
  bool is_key_info_seed_valid;

  std::map<std::string, CurrentReconcileInfo> current_reconciles;
};

}  // namespace ledger

#endif  // BAT_LEDGER_REWARDS_INTERNALS_INFO_
