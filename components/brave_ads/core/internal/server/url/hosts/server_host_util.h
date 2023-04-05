/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVER_URL_HOSTS_SERVER_HOST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVER_URL_HOSTS_SERVER_HOST_UTIL_H_

#include <string>

namespace brave_ads::server {

// Use for requests that are not user-specific and do not process personal data
std::string GetStaticHost();

// Only used by the /v1/getstate endpoint
std::string GetGeoHost();

// Use for requests that include the wallet ID and therefore fully identify the
// user
std::string GetNonAnonymousHost();

// Use for requests that must not include the wallet ID or allow Brave to link
// the request to a wallet in any other way
std::string GetAnonymousHost();

// Use for search requests that must not include the wallet ID or allow Brave
// to link the request to a wallet in any other way
std::string GetAnonymousSearchHost();

}  // namespace brave_ads::server

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVER_URL_HOSTS_SERVER_HOST_UTIL_H_
