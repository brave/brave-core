/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/studies_user_data.h"

#include <utility>

#include "base/values.h"
#include "bat/ads/internal/features/features.h"

namespace ads {
namespace user_data {

base::DictionaryValue GetStudies() {
  base::ListValue list;

  base::FieldTrial::ActiveGroups studies = features::GetStudies();
  for (const auto& study : studies) {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetStringKey("name", study.trial_name);
    dictionary.SetStringKey("group", study.group_name);

    list.Append(std::move(dictionary));
  }

  base::DictionaryValue user_data;
  user_data.SetKey("studies", std::move(list));

  return user_data;
}

}  // namespace user_data
}  // namespace ads
