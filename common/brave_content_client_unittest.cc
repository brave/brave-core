/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_content_client.h"

#include "content/common/url_schemes.h"
#include "content/public/common/url_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_util.h"

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
