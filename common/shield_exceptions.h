/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_SHIELD_EXCEPTIONS_H_
#define BRAVE_COMMON_SHIELD_EXCEPTIONS_H_

class GURL;

namespace brave {

bool IsUAWhitelisted(const GURL& gurl);
bool IsBlockedResource(const GURL& gurl);
bool IsWhitelistedCookieException(const GURL& firstPartyOrigin,
                                  const GURL& subresourceUrl,
                                  bool allow_google_auth);
bool IsWhitelistedFingerprintingException(const GURL& firstPartyOrigin,
                                          const GURL& subresourceUrl);

}  // namespace brave

#endif  // BRAVE_COMMON_SHIELD_EXCEPTIONS_H_
