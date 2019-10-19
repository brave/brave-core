/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/promotion.h"

namespace brave_rewards {

Promotion::Promotion() : expires_at(0) {}

Promotion::~Promotion() {}

Promotion::Promotion(const Promotion &properties) {
  amount = properties.amount;
  promotion_id = properties.promotion_id;
  expires_at = properties.expires_at;
  type = properties.type;
}

}  // namespace brave_rewards
