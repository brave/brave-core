/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Only run timestamp fetcher for brave sync account
#define BRAVE_START_GET_ACCESS_TOKEN                                       \
  if (refresh_token_ == "dummy_refresh_token")                             \
    url_loader_ =                                                          \
        CreateURLLoader(GURL("http://localhost:8295/v2/timestamp"),        \
                        MakeGetAccessTokenBody(client_id_, client_secret_, \
                                               refresh_token_, scopes_));  \
  else

#include "../../../../google_apis/gaia/oauth2_access_token_fetcher_impl.cc"

#undef BRAVE_START_GET_ACCESS_TOKEN
