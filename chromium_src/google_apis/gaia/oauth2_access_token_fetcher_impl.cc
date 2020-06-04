/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "url/gurl.h"

namespace {
const char kBraveSyncTimestampSuffix[] = "v2/timestamp";
// Use switches::kSyncServiceURL actual value to prevent dependency cycle
const char kSyncServiceURL[] = "sync-url";
const char kBraveSyncAccountRefreshToken[] = "dummy_refresh_token";

GURL GetSyncServiceTimestampUrl() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  std::string url_str = command_line->GetSwitchValueASCII(kSyncServiceURL);
  if (url_str.empty()) {
    LOG(ERROR) << "Empty switches::kSyncServiceURL value";
    return GURL();
  }
  GURL url(url_str);
  return url.Resolve(kBraveSyncTimestampSuffix);
}
}  // namespace

// Only run timestamp fetcher for brave sync account
#define BRAVE_START_GET_ACCESS_TOKEN                                       \
  if (refresh_token_ == kBraveSyncAccountRefreshToken)                     \
    url_loader_ =                                                          \
        CreateURLLoader(GURL(GetSyncServiceTimestampUrl()),                \
                        MakeGetAccessTokenBody(client_id_, client_secret_, \
                                               refresh_token_, scopes_));  \
  else

#include "../../../../google_apis/gaia/oauth2_access_token_fetcher_impl.cc"

#undef BRAVE_START_GET_ACCESS_TOKEN
