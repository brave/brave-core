// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/fonts/shaping/ng_shape_cache.h"
#include "third_party/blink/renderer/platform/text/text_direction.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {
namespace {

ShapeCacheKey CreateShapeCacheKey(float specified_size) {
  const String text("A");
  return ShapeCacheKey(text, 0, text.length(), g_null_atom, {},
                       TextDirection::kLtr, specified_size);
}

TEST(ShapeCacheKeyTest, SpecifiedSizeAffectsEquality) {
  const ShapeCacheKey size_12 = CreateShapeCacheKey(12.f);
  const ShapeCacheKey size_12_0003 = CreateShapeCacheKey(12.0003f);

  EXPECT_FALSE(size_12 == size_12_0003);
}

}  // namespace
}  // namespace blink
