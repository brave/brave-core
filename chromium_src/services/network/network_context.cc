/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/network_context.h"

#include <map>
#include <vector>

#include "url/gurl.h"
#include "components/content_settings/core/common/content_settings_pattern.h"

bool IsWhitelistedCookieException(const GURL& request_url,
                                  const GURL& first_party_url) {
  // Check with the security team before adding exceptions.

  // 1st-party-dependent whitelist
  std::map<GURL, std::vector<ContentSettingsPattern>> whitelist_patterns = {
    {
      GURL("https://www.sliver.tv/"),
      std::vector<ContentSettingsPattern>({
        ContentSettingsPattern::FromString("https://*.thetatoken.org:8700/*"),
      }),
    }
  };

  std::map<GURL, std::vector<ContentSettingsPattern>>::iterator i =
      whitelist_patterns.find(first_party_url.GetOrigin());
  if (i == whitelist_patterns.end()) {
    return false;
  }
  std::vector<ContentSettingsPattern> &exceptions = i->second;
  return std::any_of(exceptions.begin(), exceptions.end(),
      [&request_url](const ContentSettingsPattern& pattern) {
        return pattern.Matches(request_url);
      });
}

GURL GetURLForCookieAccess(const net::URLRequest& request) {
  if (!request.site_for_cookies().is_empty())
    return request.site_for_cookies();

  if (request.network_isolation_key().IsFullyPopulated()) {
    GURL origin(request.network_isolation_key().ToString());
    if (origin.is_valid())
      return origin;
  }

  if (request.top_frame_origin().has_value())
    return request.top_frame_origin()->GetURL();

  return GURL();
}

#define BRAVE_ON_CAN_GET_COOKIES_INTERNAL \
return allowed_from_caller && \
       (network_context_->cookie_manager() \
          ->cookie_settings() \
          .IsCookieAccessAllowed( \
              request.url(), \
              GetURLForCookieAccess(request)) || \
        IsWhitelistedCookieException(request.url(), \
                                     GetURLForCookieAccess(request)));

#define BRAVE_ON_CAN_SET_COOKIES_INTERNAL BRAVE_ON_CAN_GET_COOKIES_INTERNAL

#include "../../../../services/network/network_context.cc"  // NOLINT
#undef BRAVE_ON_CAN_GET_COOKIES_INTERNAL
#undef BRAVE_ON_CAN_SET_COOKIES_INTERNAL
