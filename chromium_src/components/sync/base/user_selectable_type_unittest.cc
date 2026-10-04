/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/sync/base/user_selectable_type_unittest.cc>

namespace syncer {
namespace {

TEST_F(UserSelectableTypeTest, AIChatMapsToUserSelectableType) {
  EXPECT_EQ(GetUserSelectableTypeFromDataType(AI_CHAT_CONVERSATION),
            UserSelectableType::kAIChat);
}

TEST_F(UserSelectableTypeTest, AIChatTypeName) {
  EXPECT_STREQ(GetUserSelectableTypeName(UserSelectableType::kAIChat),
               "aiChat");
}

TEST_F(UserSelectableTypeTest, AIChatFromString) {
  EXPECT_EQ(GetUserSelectableTypeFromString("aiChat"),
            UserSelectableType::kAIChat);
}

TEST_F(UserSelectableTypeTest, AIChatRoundTripsThroughValueList) {
  const UserSelectableTypeSet input{UserSelectableType::kAIChat};
  const base::ListValue value_list = UserSelectableTypeSetToValueList(input);
  EXPECT_EQ(value_list, base::ListValue().Append("aiChat"));
  EXPECT_EQ(ValueListToUserSelectableTypeSet(value_list), input);
}

}  // namespace
}  // namespace syncer
