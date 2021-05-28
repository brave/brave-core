/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_COOKIE_SETTINGS_GET_COOKIES_SETTINGS_INTERNAL      \
  if (setting == CONTENT_SETTING_SESSION_ONLY && !block_third && \
      ShouldBlockThirdPartyCookies() &&                          \
      !first_party_url.SchemeIs(extension_scheme_)) {            \
    block_third = true;                                          \
  }

#include "../../../../../../components/content_settings/core/browser/cookie_settings.cc"

#undef BRAVE_COOKIE_SETTINGS_GET_COOKIES_SETTINGS_INTERNAL
