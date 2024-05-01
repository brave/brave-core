/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_USER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_USER_H_

#include <string>

namespace brave_rewards::internal::uphold {

struct User {
  User();

  User(const User&);
  User& operator=(const User&);

  std::string name;
  std::string member_id;
  std::string country_id;
  bool bat_not_allowed = true;
};

}  // namespace brave_rewards::internal::uphold

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_USER_H_
