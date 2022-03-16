/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/studies_user_data.h"

#include "base/values.h"
#include "bat/ads/internal/features/features.h"

namespace ads {
namespace user_data {

base::DictionaryValue GetStudies() {
  base::ListValue list;

  base::FieldTrial::ActiveGroups studies = features::GetStudies();
  for (const auto& study : studies) {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetKey("name", base::Value(study.trial_name));
    dictionary.SetKey("group", base::Value(study.group_name));

    list.Append(dictionary.Clone());
  }

  base::DictionaryValue user_data;
  user_data.SetKey("studies", list.Clone());

  return user_data;
}

}  // namespace user_data
}  // namespace ads
