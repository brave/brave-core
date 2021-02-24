/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_experiment_dto_user_data.h"

#include <string>
#include <utility>

#include "base/optional.h"
#include "bat/ads/internal/features/features.h"

namespace ads {
namespace dto {
namespace user_data {

base::DictionaryValue GetExperiment() {
  base::DictionaryValue user_data;

  if (!features::HasActiveStudy()) {
    user_data.SetKey("experiment", base::Value(base::Value::Type::DICTIONARY));
  } else {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    const base::Optional<std::string> study = features::GetStudy();
    if (study.has_value() && !study->empty()) {
      dictionary.SetKey("name", base::Value(study.value()));
    }

    const base::Optional<std::string> group = features::GetGroup();
    if (group.has_value() && !group->empty()) {
      dictionary.SetKey("group", base::Value(group.value()));
    }

    user_data.SetKey("experiment", std::move(dictionary));
  }

  return user_data;
}

}  // namespace user_data
}  // namespace dto
}  // namespace ads
