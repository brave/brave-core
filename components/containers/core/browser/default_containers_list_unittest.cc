// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/default_containers_list.h"

#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider_manager.h"

namespace containers {

class DefaultContainersListTest : public testing::Test {
 public:
  DefaultContainersListTest() = default;

  void TearDown() override { ui::ColorProviderManager::ResetForTesting(); }
};

TEST_F(DefaultContainersListTest, CreatesDefaultContainersList) {
  std::vector<mojom::ContainerPtr> list = CreateDefaultContainersList();
  ASSERT_EQ(list.size(), 4u);
  ASSERT_EQ(kDefaultContainerIds.size(), list.size());

  EXPECT_EQ(list[0]->id, "0053d88e-7b26-4bfa-9175-783e1bfcba97");
  EXPECT_EQ(list[0]->icon, mojom::Icon::kPersonal);
  EXPECT_FALSE(list[0]->name.empty());
  EXPECT_NE(list[0]->background_color, SK_ColorTRANSPARENT);

  EXPECT_EQ(list[1]->id, "2c2777a1-80b3-4b26-b629-ae1aefd9f272");
  EXPECT_EQ(list[1]->icon, mojom::Icon::kWork);
  EXPECT_FALSE(list[1]->name.empty());
  EXPECT_NE(list[1]->background_color, SK_ColorTRANSPARENT);

  EXPECT_EQ(list[2]->id, "cfc3ee90-a163-4bd0-99ff-f0a472dda804");
  EXPECT_EQ(list[2]->icon, mojom::Icon::kSocial);
  EXPECT_FALSE(list[2]->name.empty());
  EXPECT_NE(list[2]->background_color, SK_ColorTRANSPARENT);

  EXPECT_EQ(list[3]->id, "f5d49086-1a5f-42b0-83fa-58682e5b8ba5");
  EXPECT_EQ(list[3]->icon, mojom::Icon::kSchool);
  EXPECT_FALSE(list[3]->name.empty());
  EXPECT_NE(list[3]->background_color, SK_ColorTRANSPARENT);
}

}  // namespace containers
