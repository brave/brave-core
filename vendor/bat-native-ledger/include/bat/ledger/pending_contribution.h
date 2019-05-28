/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_PENDING_CONTRIBUTION_HANDLER_
#define BAT_LEDGER_PENDING_CONTRIBUTION_HANDLER_

#include <string>
#include <vector>

#include "bat/ledger/export.h"
#include "bat/ledger/publisher_info.h"

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
  REWARDS_CATEGORY category;
};

LEDGER_EXPORT struct PendingContributionList {
  PendingContributionList();
  ~PendingContributionList();
  PendingContributionList(const PendingContributionList& data);

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::vector<PendingContribution> list_;
};

LEDGER_EXPORT struct PendingContributionInfo {
  PendingContributionInfo();
  PendingContributionInfo(const PendingContributionInfo& info);
  ~PendingContributionInfo();

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  std::string publisher_key;
  REWARDS_CATEGORY category;
  bool verified;
  std::string name;
  std::string url;
  std::string provider;
  std::string favicon_url;
  double amount = 0;
  uint64_t added_date = 0;
  std::string viewing_id;
  uint64_t expiration_date = 0;
};

using PendingContributionInfoList = std::vector<PendingContributionInfo>;

}  // namespace ledger

#endif  // BAT_LEDGER_PENDING_CONTRIBUTION_HANDLER_
