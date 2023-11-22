/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_EVENT_LOG_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_EVENT_LOG_UTIL_H_

#include <string>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

namespace brave_rewards::internal {
namespace log {
std::string GetEventLogKeyForLinkingResult(mojom::ConnectExternalWalletResult);
}  // namespace log
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_EVENT_LOG_UTIL_H_
