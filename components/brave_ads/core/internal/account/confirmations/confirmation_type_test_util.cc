/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type_test_util.h"

#include "base/check_op.h"
#include "base/rand_util.h"

namespace brave_ads::test {

std::vector<ConfirmationType>
BuildConfirmationTypeForCountAndIntersperseOtherTypes(
    const ConfirmationType confirmation_type,
    const int count) {
  CHECK_GT(count, 0);

  const int confirmation_type_max_value =
      static_cast<int>(ConfirmationType::kMaxValue);

  std::vector<ConfirmationType> confirmation_types;
  confirmation_types.reserve(count + confirmation_type_max_value - 1);

  for (int i = 0; i < count; ++i) {
    confirmation_types.push_back(confirmation_type);
  }

  // Sprinkle in one of each confirmation type, other than `confirmation_type`.
  for (int i = 0; i < confirmation_type_max_value; ++i) {
    const auto other_confirmation_type = static_cast<ConfirmationType>(i);
    if (other_confirmation_type == confirmation_type) {
      continue;
    }

    // Sprinkles on ice cream, sprinkles on cakes, sprinkle-covered donuts,
    // cupcakes, or even confirmation types.
    const int random_index =
        base::RandInt(0, static_cast<int>(confirmation_types.size()));
    confirmation_types.insert(confirmation_types.cbegin() + random_index,
                              other_confirmation_type);
  }

  return confirmation_types;
}

}  // namespace brave_ads::test
