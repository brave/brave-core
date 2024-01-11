/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_URL_HELPERS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_URL_HELPERS_H_

#include <initializer_list>
#include <string>
#include <utility>

#include "url/gurl.h"

namespace brave_rewards::internal {

// A collection of URL building helpers.
struct URLHelpers {
  // Sets a list of query paremters on a URL.
  static GURL SetQueryParameters(
      GURL url,
      std::initializer_list<std::pair<std::string, std::string>> pairs);

  // Resolves a relative URL with a list of components that are joined
  // together. If there is only one part to resolve, then use GURL::Resolve
  // instead.
  static GURL Resolve(GURL url, std::initializer_list<std::string> parts);
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_URL_HELPERS_H_
