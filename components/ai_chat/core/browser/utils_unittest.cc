/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/utils.h"

#include <string_view>
#include <utility>
#include <vector>

#include "base/memory/discardable_memory_allocator.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image_unittest_util.h"
#include "url/gurl.h"

namespace ai_chat {

class AIChatUtilsUnitTest : public ::testing::Test {
 public:
  void SetUp() override {
    base::DiscardableMemoryAllocator::SetInstance(
        &discardable_memory_allocator_);
  }

  void TearDown() override {
    base::DiscardableMemoryAllocator::SetInstance(nullptr);
  }

 private:
  base::TestDiscardableMemoryAllocator discardable_memory_allocator_;
};

TEST_F(AIChatUtilsUnitTest, IsBraveSearchSERP) {
  EXPECT_TRUE(IsBraveSearchSERP(GURL("https://search.brave.com/search?q=foo")));
  // Missing or wrong path.
  EXPECT_FALSE(IsBraveSearchSERP(GURL("https://search.brave.com?q=foo")));
  EXPECT_FALSE(
      IsBraveSearchSERP(GURL("https://search.brave.com/test.html?q=foo")));
  // Missing or wrong query parameter.
  EXPECT_FALSE(IsBraveSearchSERP(GURL("https://search.brave.com/search")));
  EXPECT_FALSE(IsBraveSearchSERP(GURL("https://search.brave.com/search?t=t")));
  // HTTP scheme.
  EXPECT_FALSE(IsBraveSearchSERP(GURL("http://search.brave.com/search?q=foo")));
  // Wrong host.
  EXPECT_FALSE(IsBraveSearchSERP(GURL("https://brave.com/search?q=foo")));
}

TEST_F(AIChatUtilsUnitTest, ScaleDownBitmap) {
  const std::vector<std::pair<int, int>> large_test_dimensions = {
      {2560, 1440}, {1024, 1440}, {2560, 768}};
  for (auto& [width, height] : large_test_dimensions) {
    SCOPED_TRACE(testing::Message() << width << "x" << height);
    const auto bitmap = gfx::test::CreateBitmap(width, height);
    const auto scaled_bitmap = ScaleDownBitmap(bitmap);
    EXPECT_EQ(scaled_bitmap.width(), 1024);
    EXPECT_EQ(scaled_bitmap.height(), 768);
  }

  const std::vector<std::pair<int, int>> no_change_test_dimensions = {
      {1024, 768}, {1024, 720}, {960, 768}, {960, 720}};
  for (auto& [width, height] : no_change_test_dimensions) {
    SCOPED_TRACE(testing::Message() << width << "x" << height);
    const auto bitmap = gfx::test::CreateBitmap(width, height);
    const auto scaled_bitmap = ScaleDownBitmap(bitmap);
    EXPECT_TRUE(gfx::test::AreBitmapsEqual(bitmap, scaled_bitmap));
  }
}

}  // namespace ai_chat
