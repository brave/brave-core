// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/containers_icon_generator.h"

#include <unordered_map>
#include <utility>

#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

using ContainersIconGeneratorUnitTest = testing::Test;

TEST_F(ContainersIconGeneratorUnitTest, GetVectorIconFromIconType_BasicIcons) {
  const std::unordered_map<mojom::Icon, const gfx::VectorIcon&>
      icon_to_vector_icon_map(
          {{mojom::Icon::kPersonal, kLeoContainerPersonalIcon},
           {mojom::Icon::kWork, kLeoContainerWorkIcon},
           {mojom::Icon::kShopping, kLeoContainerShoppingIcon},
           {mojom::Icon::kSocial, kLeoContainerSocialIcon},
           {mojom::Icon::kEvents, kLeoContainerEventsIcon},
           {mojom::Icon::kBanking, kLeoContainerBankingIcon},
           {mojom::Icon::kStar, kLeoContainerStarIcon},
           {mojom::Icon::kTravel, kLeoContainerTravelIcon},
           {mojom::Icon::kSchool, kLeoContainerSchoolIcon},
           {mojom::Icon::kPrivate, kLeoContainerPrivateIcon},
           {mojom::Icon::kMessaging, kLeoContainerMessagingIcon}});

  for (int i = std::to_underlying(mojom::Icon::kMinValue);
       i <= std::to_underlying(mojom::Icon::kMaxValue); ++i) {
    mojom::Icon icon = static_cast<mojom::Icon>(i);
    auto it = icon_to_vector_icon_map.find(icon);
    EXPECT_NE(it, icon_to_vector_icon_map.end());
    EXPECT_EQ(&GetVectorIconFromIconType(icon), &it->second);
  }
}

TEST_F(ContainersIconGeneratorUnitTest,
       GetVectorIconFromIconType_DefaultValue) {
  // Test that the default value returns the default icon without crashing.
  EXPECT_EQ(&GetVectorIconFromIconType(mojom::Icon::kDefault),
            &kLeoContainerPersonalIcon);
}

TEST_F(ContainersIconGeneratorUnitTest,
       GetVectorIconFromIconType_OutOfRangeValue) {
  // Test that an out-of-range value returns the default icon without crashing.
  EXPECT_EQ(&GetVectorIconFromIconType(static_cast<mojom::Icon>(
                std::to_underlying(mojom::Icon::kMaxValue) + 1)),
            &kLeoContainerPersonalIcon);
  EXPECT_EQ(&GetVectorIconFromIconType(static_cast<mojom::Icon>(
                std::to_underlying(mojom::Icon::kMinValue) - 1)),
            &kLeoContainerPersonalIcon);
}

}  // namespace containers
