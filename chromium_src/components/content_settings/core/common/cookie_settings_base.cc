/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/core/common/cookie_settings_base.h"

#define BRAVE_IS_COOKIE_ACCESS_ALLOWED \
  GURL first_party_url = site_for_cookies; \
  if (first_party_url.is_empty() && top_frame_origin) { \
    first_party_url = top_frame_origin->GetURL(); \
  } \
  ContentSetting content_setting; \
  GetCookieSettingInternal( \
      url, first_party_url, \
      IsThirdPartyRequest(url, site_for_cookies), nullptr, &content_setting); \
  return IsAllowed(content_setting);

#include "../../../../../components/content_settings/core/common/cookie_settings_base.cc"  // NOLINT
#undef BRAVE_IS_COOKIE_ACCESS_ALLOWED
