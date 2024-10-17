/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "components/permissions/permissions_client.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace permissions {

using PermissionsClientUnitTest = testing::Test;

TEST_F(PermissionsClientUnitTest, BraveCanBypassEmbeddingOriginCheck) {
  auto* client = PermissionsClient::Get();
  ASSERT_TRUE(client);

  struct {
    GURL requesting_origin;
    GURL requesting_origin_with_port;
    ContentSettingsType type;
  } cases[] = {
      {GURL("https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A"),
       GURL("https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A:123"),
       ContentSettingsType::BRAVE_ETHEREUM},
      {GURL("https://test.com__BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"),
       GURL("https://"
            "test.com__BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8:123"),
       ContentSettingsType::BRAVE_SOLANA}};

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    GURL embedding_origin("https://test.com");
    EXPECT_TRUE(client->BraveCanBypassEmbeddingOriginCheck(
        cases[i].requesting_origin, embedding_origin, cases[i].type))
        << "case: " << i;

    GURL embedding_origin_with_port("https://test.com:123");
    EXPECT_TRUE(client->BraveCanBypassEmbeddingOriginCheck(
        cases[i].requesting_origin_with_port, embedding_origin_with_port,
        cases[i].type))
        << "case: " << i;

    EXPECT_FALSE(client->BraveCanBypassEmbeddingOriginCheck(
        cases[i].requesting_origin, embedding_origin,
        ContentSettingsType::GEOLOCATION))
        << "case: " << i;
  }
}

}  // namespace permissions
