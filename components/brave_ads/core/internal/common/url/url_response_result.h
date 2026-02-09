/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_RESPONSE_RESULT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_RESPONSE_RESULT_H_

#include <string>
#include <utility>

#include "base/types/expected.h"

namespace brave_ads {

struct UrlResponseErrorInfo {
  std::string message;
  bool should_retry = false;
};

template <typename T>
using UrlResponseResult = base::expected<T, UrlResponseErrorInfo>;

inline auto UrlResponseError(UrlResponseErrorInfo error) {
  return base::unexpected(std::move(error));
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_RESPONSE_RESULT_H_
