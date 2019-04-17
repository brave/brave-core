/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/media/vimeo.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=VimeoTest.*

namespace braveledger_media {

class VimeoTest : public testing::Test {
};

TEST(VimeoTest, GetLinkType) {
  // TODO(nejczdovc): change me
  std::string result = Vimeo::GetLinkType("https://vimeo.com/video/32342");
  ASSERT_EQ(result, "vimeo");
}

}  // namespace braveledger_media
