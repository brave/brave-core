/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_GRANT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_GRANT_H_

#include <string>

namespace brave_rewards {

struct Grant {
  Grant();
  ~Grant();
  Grant(const Grant& properties);

  std::string altcurrency;
  std::string probi;
  std::string promotionId;
  uint64_t expiryTime;
  std::string type;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_GRANT_H_
