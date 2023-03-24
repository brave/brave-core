/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_CAPABILITIES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_CAPABILITIES_H_

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_rewards::core {
namespace uphold {

struct Capabilities {
  absl::optional<bool> can_receive;
  absl::optional<bool> can_send;
};

}  // namespace uphold
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_CAPABILITIES_H_
