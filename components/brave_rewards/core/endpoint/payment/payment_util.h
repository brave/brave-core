/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_PAYMENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_PAYMENT_UTIL_H_

#include <string>

namespace brave_rewards::core {
namespace endpoint {
namespace payment {

std::string GetServerUrl(const std::string& path);

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PAYMENT_PAYMENT_UTIL_H_
