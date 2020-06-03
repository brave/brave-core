/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// TODO(darkdh): This can also be overload by loading a json config file by
// switch "gaia-config"

#define BRAVE_GAIA_URLS                                           \
  oauth2_token_url_ = GURL("http://localhost:8295/v2/timestamp"); \
  oauth_user_info_url_ =                                          \
      GURL("https://no-thanks.invalid").Resolve(kOAuthUserInfoUrlSuffix);

#include "../../../../google_apis/gaia/gaia_urls.cc"

#undef BRAVE_GAIA_URLS
