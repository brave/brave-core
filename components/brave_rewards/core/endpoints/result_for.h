/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_RESULT_FOR_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_RESULT_FOR_H_

namespace brave_rewards::internal::endpoints {

template <typename>
inline constexpr bool dependent_false_v = false;

template <typename Endpoint>
struct ResultFor {
  static_assert(dependent_false_v<Endpoint>,
                "Please explicitly specialize ResultFor<> for your endpoint!");
};

}  // namespace brave_rewards::internal::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_RESULT_FOR_H_
