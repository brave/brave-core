/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_UTIL_INTERNAL_H_

#include <string_view>

class GURL;

namespace brave_ads {

bool HasSearchQuery(const GURL& url);
bool ShouldSupportInternalUrl(const GURL& url);

bool HostHasRegistryControlledDomain(std::string_view host);

bool DoesETLDPlusOneContainWildcards(const GURL& url);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_UTIL_INTERNAL_H_
