/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_GET_TIMESTAMP_URL                                       \
  profile_sync_service->GetSyncServiceUrlForDebugging().Resolve("v2/" \
                                                                "timestamp")
#define BRAVE_GET_AUTH_URL \
  profile_sync_service->GetSyncServiceUrlForDebugging().Resolve("v2/auth")

#define BRAVE_INITIALIZE_PROFILE                                       \
  test_url_loader_factory_.AddResponse(BRAVE_GET_TIMESTAMP_URL.spec(), \
                                       R"({"timestamp": "123456"})");  \
  test_url_loader_factory_.AddResponse(                                \
      BRAVE_GET_AUTH_URL.spec(),                                       \
      R"({"access_token": "at1", "expires_in": 3600})");               \
  profile_sync_service->SetURLLoaderFactoryForTest(                    \
      test_url_loader_factory_.GetSafeWeakWrapper());

#define BRAVE_SET_OAUTH2_TOKEN_RESPONSE_1 \
  ProfileSyncService* profile_sync_service = GetClient(0)->service();

#define BRAVE_SET_OAUTH2_TOKEN_RESPONSE_2 \
  BRAVE_GET_AUTH_URL, std::move(response_head),
#include "../../../../../../../chrome/browser/sync/test/integration/sync_test.cc"
#undef BRAVE_GET_TIMESTAMP_URL
#undef BRAVE_GET_AUTH_URL
#undef BRAVE_INITIALIZE_PROFILE
#undef BRAVE_SET_OAUTH2_TOKEN_RESPONSE_1
#undef BRAVE_SET_OAUTH2_TOKEN_RESPONSE_2
