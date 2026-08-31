/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_content_client.h"

#include <algorithm>
#include <string>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "content/common/url_schemes.h"
#include "content/public/common/url_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_util.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/constants.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

TEST(BraveContentClientTest, AdditionalSchemesTest) {
  url::ScopedSchemeRegistryForTests scoped_registry;
  BraveContentClient content_client;
  content::SetContentClient(&content_client);
  content::ReRegisterContentSchemesForTests();

  const GURL sync_url("brave://sync");
  EXPECT_TRUE(sync_url.is_valid());
  EXPECT_TRUE(sync_url.has_host());
  EXPECT_EQ("sync", sync_url.host());

  const GURL chrome_sync_url("chrome://sync");
  EXPECT_TRUE(chrome_sync_url.is_valid());
  EXPECT_TRUE(chrome_sync_url.has_host());
  EXPECT_EQ("sync", chrome_sync_url.host());
}

#if BUILDFLAG(ENABLE_AI_CHAT)
TEST(BraveContentClientTest, LeoWorkspaceSchemeProperties) {
  url::ScopedSchemeRegistryForTests scoped_registry;
  BraveContentClient content_client;
  content::SetContentClient(&content_client);
  content::ReRegisterContentSchemesForTests();

  const std::string scheme(ai_chat::kLeoWorkspaceContentScheme);

  // Standard, so each workspace uuid is its own origin rather than an opaque
  // one, and so relative references between generated files resolve.
  EXPECT_TRUE(std::ranges::contains(url::GetStandardSchemes(), scheme));

  // Secure, or previews get no storage and count as mixed content when
  // embedded.
  EXPECT_TRUE(std::ranges::contains(url::GetSecureSchemes(), scheme));

  // Deliberately absent, and asserted so that neither can be added without a
  // deliberate decision: cors_enabled would let previews issue fetch()/XHR at
  // all rather than have them refused by connect-src, and a service worker
  // here would be registered by model-authored script.
  EXPECT_FALSE(std::ranges::contains(url::GetCorsEnabledSchemes(), scheme));
  EXPECT_FALSE(
      std::ranges::contains(content::GetServiceWorkerSchemes(), scheme));

  const GURL preview_url(
      "brave-leo-workspace://6b1b3f1e-0b7a-4f27-9a2f-2f2b9d4e5a10/index.html");
  EXPECT_TRUE(preview_url.is_valid());
  EXPECT_TRUE(preview_url.has_host());
  EXPECT_EQ("6b1b3f1e-0b7a-4f27-9a2f-2f2b9d4e5a10", preview_url.host());
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
