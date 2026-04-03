/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/sync/base/user_selectable_type_unittest.cc>

namespace syncer {
namespace {

TEST_F(UserSelectableTypeTest, AIChatMapsToUserSelectableType) {
  const auto result = GetUserSelectableTypeFromDataType(AI_CHAT_CONVERSATION);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), UserSelectableType::kAIChat);
}

TEST_F(UserSelectableTypeTest, AIChatTypeName) {
  EXPECT_STREQ(GetUserSelectableTypeName(UserSelectableType::kAIChat),
               "aiChat");
}

TEST_F(UserSelectableTypeTest, AIChatFromString) {
  const auto result = GetUserSelectableTypeFromString("aiChat");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), UserSelectableType::kAIChat);
}

}  // namespace
}  // namespace syncer
