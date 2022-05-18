/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVER_HOSTS_SERVER_HOST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVER_HOSTS_SERVER_HOST_UTIL_H_

#include <string>

namespace ads {
namespace server {

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

}  // namespace server
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVER_HOSTS_SERVER_HOST_UTIL_H_
