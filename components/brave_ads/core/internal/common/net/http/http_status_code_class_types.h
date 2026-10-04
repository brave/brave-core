/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_NET_HTTP_HTTP_STATUS_CODE_CLASS_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_NET_HTTP_HTTP_STATUS_CODE_CLASS_TYPES_H_

namespace brave_ads {

enum class HttpStatusCodeClassType {
  kNonsensical = 0,
  kInformationalResponse = 1,  // 1xx
  kSuccess = 2,                // 2xx
  kRedirection = 3,            // 3xx
  kClientError = 4,            // 4xx
  kServerError = 5             // 5xx
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_NET_HTTP_HTTP_STATUS_CODE_CLASS_TYPES_H_
