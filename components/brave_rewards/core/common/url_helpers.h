/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_URL_HELPERS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_URL_HELPERS_H_

#include <optional>
#include <string_view>
#include <utility>

#include "net/base/url_util.h"
#include "url/gurl.h"

namespace brave_rewards::internal {

// This is a utiltity wrapper call around `net::AppendOrReplaceQueryParameter`,
// that allows to add multiple parameters at once.
// e.g.
//
// url = AppendOrReplaceQueryParameters(
//     url, {{"client_id", config.zebpay_client_id()},
//           {"grant_type", "authorization_code"},
//           {"redirect_uri", "rewards://zebpay/authorization"},
//           {"response_type", "code"},
//           {"scope", "openid profile"},
//           {"state", oauth_info_.one_time_string}});
//
template <size_t N>
GURL AppendOrReplaceQueryParameters(
    GURL url,
    std::pair<std::string_view, std::optional<std::string_view>> (&&data)[N]) {
  static_assert(
      N > 1,
      "This function should be used only with more than one parameter. "
      "Otherwise just call net::AppendOrReplaceQueryParameter directly.");
  for (const auto& [name, value] : data) {
    url = net::AppendOrReplaceQueryParameter(url, name, value);
  }
  return url;
}

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_URL_HELPERS_H_
