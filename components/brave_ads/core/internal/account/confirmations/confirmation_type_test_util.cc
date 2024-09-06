/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_type_test_util.h"

#include "base/check_op.h"
#include "base/rand_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::test {

std::vector<mojom::ConfirmationType>
BuildConfirmationTypeForCountAndIntersperseOtherTypes(
    const mojom::ConfirmationType mojom_confirmation_type,
    const int count) {
  CHECK_GT(count, 0);

  const int confirmation_type_max_value =
      static_cast<int>(mojom::ConfirmationType::kMaxValue);

  std::vector<mojom::ConfirmationType> mojom_confirmation_types;
  mojom_confirmation_types.reserve(count + confirmation_type_max_value - 1);

  for (int i = 0; i < count; ++i) {
    mojom_confirmation_types.push_back(mojom_confirmation_type);
  }

  // Sprinkle in one of each confirmation type, other than `confirmation_type`.
  for (int i = 0; i < confirmation_type_max_value; ++i) {
    const auto other_mojom_confirmation_type =
        static_cast<mojom::ConfirmationType>(i);
    if (other_mojom_confirmation_type == mojom_confirmation_type) {
      continue;
    }

    // Sprinkles on ice cream, sprinkles on cakes, sprinkle-covered donuts,
    // cupcakes, or even confirmation types.
    const int random_index =
        base::RandInt(0, static_cast<int>(mojom_confirmation_types.size()));
    mojom_confirmation_types.insert(
        mojom_confirmation_types.cbegin() + random_index,
        other_mojom_confirmation_type);
  }

  return mojom_confirmation_types;
}

}  // namespace brave_ads::test
