/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permissions_client.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace permissions {

using PermissionsClientUnitTest = testing::Test;

TEST_F(PermissionsClientUnitTest, BraveCanBypassEmbeddingOriginCheck) {
  auto* client = PermissionsClient::Get();
  ASSERT_TRUE(client);

  GURL requesting_origin(
      "https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A");
  GURL embedding_origin("https://test.com");
  EXPECT_TRUE(client->BraveCanBypassEmbeddingOriginCheck(
      requesting_origin, embedding_origin,
      ContentSettingsType::BRAVE_ETHEREUM));

  GURL requesting_origin_with_port(
      "https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A:123");
  GURL embedding_origin_with_port("https://test.com:123");
  EXPECT_TRUE(client->BraveCanBypassEmbeddingOriginCheck(
      requesting_origin_with_port, embedding_origin_with_port,
      ContentSettingsType::BRAVE_ETHEREUM));

  EXPECT_FALSE(client->BraveCanBypassEmbeddingOriginCheck(
      requesting_origin, GURL("https://test1.com"),
      ContentSettingsType::BRAVE_ETHEREUM));

  EXPECT_FALSE(client->BraveCanBypassEmbeddingOriginCheck(
      requesting_origin, embedding_origin, ContentSettingsType::GEOLOCATION));
}

}  // namespace permissions
