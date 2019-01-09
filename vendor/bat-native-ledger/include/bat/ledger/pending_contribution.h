/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_PENDING_CONTRIBUTION_HANDLER_
#define BAT_LEDGER_PENDING_CONTRIBUTION_HANDLER_

#include <string>
#include <vector>

#include "bat/ledger/export.h"

namespace ledger {

LEDGER_EXPORT struct PendingContribution {
  PendingContribution();
  ~PendingContribution();
  PendingContribution(const PendingContribution& data);

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::string publisher_key;
  double amount = 0;
  uint64_t added_date = 0;
  std::string viewing_id;
  PUBLISHER_CATEGORY category;
};

LEDGER_EXPORT struct PendingContributionList {
  PendingContributionList();
  ~PendingContributionList();
  PendingContributionList(const PendingContributionList& data);

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::vector<PendingContribution> list_;
};

}  // namespace ledger

#endif  // BAT_LEDGER_PENDING_CONTRIBUTION_HANDLER_
