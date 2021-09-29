/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_studies_dto_user_data.h"

#include <utility>

#include "base/values.h"
#include "bat/ads/internal/features/features.h"

namespace ads {
namespace dto {
namespace user_data {

base::DictionaryValue GetStudies() {
  base::Value list(base::Value::Type::LIST);

  base::FieldTrial::ActiveGroups studies = features::GetStudies();
  for (const auto& study : studies) {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetKey("name", base::Value(study.trial_name));
    dictionary.SetKey("group", base::Value(study.group_name));

    list.Append(std::move(dictionary));
  }

  base::DictionaryValue user_data;
  user_data.SetKey("studies", std::move(list));

  return user_data;
}

}  // namespace user_data
}  // namespace dto
}  // namespace ads
