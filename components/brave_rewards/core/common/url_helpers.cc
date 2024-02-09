/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/common/url_helpers.h"

#include "base/strings/strcat.h"
#include "net/base/url_util.h"

namespace brave_rewards::internal {

GURL URLHelpers::SetQueryParameters(
    GURL url,
    std::initializer_list<std::pair<std::string, std::string>> pairs) {
  for (auto& [name, value] : pairs) {
    url = net::AppendOrReplaceQueryParameter(url, name, value);
  }
  return url;
}

GURL URLHelpers::Resolve(GURL url, std::initializer_list<std::string> parts) {
  return url.Resolve(base::StrCat(std::move(parts)));
}

}  // namespace brave_rewards::internal
