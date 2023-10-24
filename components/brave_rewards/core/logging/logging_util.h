/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_LOGGING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_LOGGING_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

namespace brave_rewards::internal {

bool ShouldLogHeader(const std::string& header);

std::string UrlRequestToString(const std::string& url,
                               const std::vector<std::string>& headers,
                               const std::string& content,
                               const std::string& content_type,
                               const mojom::UrlMethod method);

// DEPRECATED (use LogUrlResponse)
std::string UrlResponseToString(const char* func,
                                const mojom::UrlResponse& response);

void LogUrlResponse(const char* func,
                    const mojom::UrlResponse& response,
                    const bool long_response = false);

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_LOGGING_UTIL_H_
