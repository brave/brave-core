// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_NET_STATIC_REDIRECT_HELPER_STATIC_REDIRECT_HELPER_H_
#define BRAVE_NET_STATIC_REDIRECT_HELPER_STATIC_REDIRECT_HELPER_H_

#include "url/gurl.h"

namespace brave {

inline constexpr char kSafeBrowsingTestingEndpoint[] = "test.safebrowsing.com";

void StaticRedirectHelper(const GURL& request_url, GURL* new_url);

void SetSafeBrowsingEndpointForTesting(bool testing);

}  // namespace brave

#endif  // BRAVE_NET_STATIC_REDIRECT_HELPER_STATIC_REDIRECT_HELPER_H_
