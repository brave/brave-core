/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_AUTO_CONTRIBUTE_PROPS_
#define BAT_LEDGER_AUTO_CONTRIBUTE_PROPS_

#include <string>

namespace ledger {

LEDGER_EXPORT struct AutoContributeProps {
  AutoContributeProps();
  ~AutoContributeProps();
  AutoContributeProps(const AutoContributeProps& properties) = default;

  const std::string ToJson() const;
  bool loadFromJson(const std::string& json);

  bool enabled_contribute;
  uint64_t contribution_min_time;
  int32_t contribution_min_visits;
  bool contribution_non_verified;
  bool contribution_videos;
  uint64_t reconcile_stamp;
};

}  // namespace ledger

#endif  // BAT_LEDGER_AUTO_CONTRIBUTION_PROPS_
