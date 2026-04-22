// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/temporary_container.h"

#include <algorithm>
#include <string_view>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_manager.h"

namespace containers {

class TemporaryContainerTest : public testing::Test {
 public:
  void TearDown() override { ui::ColorProviderManager::ResetForTesting(); }
};

TEST_F(TemporaryContainerTest, IsTemporaryContainerId) {
  EXPECT_TRUE(IsTemporaryContainerId("t-container-id"));
  EXPECT_TRUE(IsTemporaryContainerId("t-a"));

  EXPECT_FALSE(IsTemporaryContainerId(""));
  EXPECT_FALSE(IsTemporaryContainerId("t"));
  EXPECT_FALSE(IsTemporaryContainerId("t-"));
  EXPECT_FALSE(IsTemporaryContainerId("T"));
  EXPECT_FALSE(IsTemporaryContainerId("T-"));
  EXPECT_FALSE(IsTemporaryContainerId("T-a"));
  EXPECT_FALSE(IsTemporaryContainerId("container-id"));
  EXPECT_FALSE(IsTemporaryContainerId("T-container-id"));
  EXPECT_FALSE(IsTemporaryContainerId("tt-container-id"));
}

TEST_F(TemporaryContainerTest, CreateTemporaryContainer) {
  auto container = CreateTemporaryContainer();
  ASSERT_TRUE(container);

  EXPECT_TRUE(IsTemporaryContainerId(container->id));
  EXPECT_GT(container->id.size(),
            std::string_view(kTemporaryContainerIdPrefix).size());

  const std::vector<std::string_view> name_parts = base::SplitStringPiece(
      container->name, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  ASSERT_EQ(name_parts.size(), 2u);
  EXPECT_TRUE(base::IsAsciiAlpha(name_parts[0][0]));
  EXPECT_TRUE(base::IsAsciiAlpha(name_parts[1][0]));
  EXPECT_EQ(name_parts[0][0], base::ToUpperASCII(name_parts[0][0]));

  EXPECT_GE(container->icon, mojom::Icon::kMinValue);
  EXPECT_LE(container->icon, mojom::Icon::kMaxValue);

  const auto* color_provider =
      ui::ColorProviderManager::Get().GetColorProviderFor(
          ui::ColorProviderKey());
  ASSERT_TRUE(color_provider);
  const SkColor expected_backgrounds[] = {
      color_provider->GetColor(nala::kColorPrimitiveRed60),
      color_provider->GetColor(nala::kColorPrimitiveOrange60),
      color_provider->GetColor(nala::kColorPrimitiveYellow60),
      color_provider->GetColor(nala::kColorPrimitiveGreen60),
      color_provider->GetColor(nala::kColorPrimitiveTeal60),
      color_provider->GetColor(nala::kColorPrimitiveBlue60),
      color_provider->GetColor(nala::kColorPrimitivePurple60),
      color_provider->GetColor(nala::kColorPrimitivePink60),
  };
  EXPECT_TRUE(
      std::ranges::contains(expected_backgrounds, container->background_color));
}

}  // namespace containers
