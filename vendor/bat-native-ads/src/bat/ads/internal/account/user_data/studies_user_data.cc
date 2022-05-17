/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/studies_user_data.h"

#include <utility>

#include "base/values.h"
#include "bat/ads/internal/studies/studies_util.h"

namespace ads {
namespace user_data {

namespace {

constexpr char kStudiesKey[] = "studies";
constexpr char kNameKey[] = "name";
constexpr char kGroupKey[] = "group";

}  // namespace

base::DictionaryValue GetStudies() {
  base::ListValue list;

  base::FieldTrial::ActiveGroups studies = GetActiveStudies();
  for (const auto& study : studies) {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetStringKey(kNameKey, study.trial_name);
    dictionary.SetStringKey(kGroupKey, study.group_name);

    list.Append(std::move(dictionary));
  }

  base::DictionaryValue user_data;
  user_data.SetKey(kStudiesKey, std::move(list));

  return user_data;
}

}  // namespace user_data
}  // namespace ads
