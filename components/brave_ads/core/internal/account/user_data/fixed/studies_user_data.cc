/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/studies_user_data.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/studies/studies_util.h"

namespace brave_ads {

namespace {

constexpr char kStudiesKey[] = "studies";
constexpr char kTrialNameKey[] = "name";
constexpr char kGroupNameKey[] = "group";

}  // namespace

base::Value::Dict BuildStudiesUserData() {
  base::Value::Dict user_data;

  if (!UserHasJoinedBraveRewards()) {
    return user_data;
  }

  base::Value::List list;

  for (const auto& active_field_trial_group :
       GetActiveFieldTrialStudyGroups()) {
    list.Append(base::Value::Dict()
                    .Set(kTrialNameKey, active_field_trial_group.trial_name)
                    .Set(kGroupNameKey, active_field_trial_group.group_name));
  }

  user_data.Set(kStudiesKey, std::move(list));

  return user_data;
}

}  // namespace brave_ads
