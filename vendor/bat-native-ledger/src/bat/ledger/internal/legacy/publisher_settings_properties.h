/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_PUBLISHER_SETTINGS_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_PUBLISHER_SETTINGS_PROPERTIES_H_

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

#include "bat/ledger/internal/legacy/report_balance_properties.h"

namespace ledger {

struct PublisherSettingsProperties {
  PublisherSettingsProperties();
  PublisherSettingsProperties(
      const PublisherSettingsProperties& properties);
  ~PublisherSettingsProperties();

  bool operator==(
      const PublisherSettingsProperties& rhs) const;

  bool operator!=(
      const PublisherSettingsProperties& rhs) const;

  uint64_t min_page_time_before_logging_a_visit;
  uint32_t min_visits_for_publisher_relevancy;
  bool allow_non_verified_sites_in_list;
  bool allow_contribution_to_videos;
  std::map<std::string, ReportBalanceProperties> monthly_balances;
  std::vector<std::string> processed_pending_publishers;
};

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_PUBLISHER_SETTINGS_PROPERTIES_H_
