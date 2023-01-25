/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "bat/ledger/internal/legacy/media/helper.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=MediaHelperTest.*

namespace braveledger_media {

class MediaHelperTest : public testing::Test {
};

TEST(MediaHelperTest, GetMediaKey) {
  // provider is missing
  std::string result = braveledger_media::GetMediaKey("key", "");
  ASSERT_EQ(result, "");

  // key is missing
  result = braveledger_media::GetMediaKey("", "youtube");
  ASSERT_EQ(result, "");

  // all ok
  result = braveledger_media::GetMediaKey("key", "youtube");
  ASSERT_EQ(result, "youtube_key");
}

TEST(MediaHelperTest, ExtractData) {
//  // string empty
  std::string result = braveledger_media::ExtractData("", "/", "!");
  ASSERT_EQ(result, "");

  // missing start
  result = braveledger_media::ExtractData("st/find/me!", "", "!");
  ASSERT_EQ(result, "st/find/me");

  // missing end
  result = braveledger_media::ExtractData("st/find/me!", "/", "");
  ASSERT_EQ(result, "find/me!");

  // all ok
  result = braveledger_media::ExtractData("st/find/me!", "/", "!");
  ASSERT_EQ(result, "find/me");
}

}  // namespace braveledger_media
