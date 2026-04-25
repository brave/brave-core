/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_NET_HTTP_HTTP_STATUS_CODE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_NET_HTTP_HTTP_STATUS_CODE_UTIL_H_

#include <optional>
#include <string>

#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code_class_types.h"

namespace brave_ads {

HttpStatusCodeClassType HttpStatusCodeClass(int http_status_code);

bool IsSuccessfulHttpStatusCode(int http_status_code);

std::optional<std::string> HttpStatusCodeToString(int http_status_code);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_NET_HTTP_HTTP_STATUS_CODE_UTIL_H_
