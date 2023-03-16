/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/studies_user_data.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/studies/studies_util.h"

namespace brave_ads::user_data {

namespace {

constexpr char kStudiesKey[] = "studies";
constexpr char kNameKey[] = "name";
constexpr char kGroupKey[] = "group";

}  // namespace

base::Value::Dict GetStudies() {
  base::Value::List list;

  const base::FieldTrial::ActiveGroups studies = GetActiveStudies();
  for (const auto& study : studies) {
    base::Value::Dict dict;

    dict.Set(kNameKey, study.trial_name);
    dict.Set(kGroupKey, study.group_name);

    list.Append(std::move(dict));
  }

  base::Value::Dict user_data;
  user_data.Set(kStudiesKey, std::move(list));

  return user_data;
}

}  // namespace brave_ads::user_data
