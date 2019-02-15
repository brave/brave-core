/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/grant.h"

namespace brave_rewards {

Grant::Grant() : expiryTime(0) {}

Grant::~Grant() {}

Grant::Grant(const Grant &properties) {
  altcurrency = properties.altcurrency;
  probi = properties.probi;
  promotionId = properties.promotionId;
  expiryTime = properties.expiryTime;
  type = properties.type;
}

}  // namespace brave_rewards
