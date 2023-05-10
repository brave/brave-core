/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_HOST_URL_HOST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_HOST_URL_HOST_UTIL_H_

#include <string>

namespace brave_ads {

// Use for requests that are not user-specific and do not process personal data.
std::string GetStaticUrlHost();

// Only used by the /v#/getstate endpoint.
std::string GetGeoUrlHost();

// Use for requests that include the wallet ID and therefore fully identify the
// user.
std::string GetNonAnonymousUrlHost();

// Use for requests that must not include the wallet ID or allow Brave to link
// the request to a wallet in any other way.
std::string GetAnonymousUrlHost();

// Use for search requests that must not include the wallet ID or allow Brave to
// link the request to a wallet in any other way.
std::string GetAnonymousSearchUrlHost();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_HOST_URL_HOST_UTIL_H_
