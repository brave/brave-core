/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RETRY_OPTIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RETRY_OPTIONS_H_

#include <type_traits>

#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_account::endpoint_client::detail {

// Retry configuration forwarded to SimpleURLLoader::SetRetryOptions().
// The defaults (no retries, RETRY_NEVER) leave the request non-retrying.
struct RetryOptions {
  int max_retries = 0;
  std::underlying_type_t<network::SimpleURLLoader::RetryMode> retry_mode =
      network::SimpleURLLoader::RETRY_NEVER;
};

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RETRY_OPTIONS_H_
