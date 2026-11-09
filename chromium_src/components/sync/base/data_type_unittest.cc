/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/containers/enum_set.h"

#include <components/sync/base/data_type_unittest.cc>

namespace syncer {
namespace {

TEST(DataTypeTest, EncryptableUserTypes) {
  EXPECT_TRUE(EncryptableUserTypes().Has(CONTACT_INFO));
  EXPECT_TRUE(EncryptableUserTypes().Has(DEVICE_INFO));
  EXPECT_TRUE(EncryptableUserTypes().Has(HISTORY));
  EXPECT_TRUE(EncryptableUserTypes().Has(PRIORITY_PREFERENCES));
}

TEST(DataTypeTest, BraveUnencryptableUserTypes) {
  EXPECT_FALSE(EncryptableUserTypes().Has(HISTORY_DELETE_DIRECTIVES));
  EXPECT_FALSE(EncryptableUserTypes().Has(NIGORI));
}

TEST(DataTypeTest, LowPriorityUserTypes) {
  EXPECT_TRUE(LowPriorityUserTypes().Has(HISTORY_DELETE_DIRECTIVES));
  EXPECT_FALSE(LowPriorityUserTypes().Has(HISTORY));
  EXPECT_TRUE(LowPriorityUserTypes().Has(USER_EVENTS));
}

// This test is used by Brave's Chromium rebasing team. It's meant to fail when
// sync types are added or removed so that the team can investigate how to
// properly support the upstream change.
TEST(DataTypeTest, DataTypeCounts) {
  EXPECT_EQ(static_cast<int>(DataTypeForHistograms::kMaxValue), 86);
}

// Brave's sync chain is always in encrypt-everything mode, so every user type
// outside EncryptableUserTypes() reaches the server as plain text. This test is
// meant to fail when a Chromium bump adds such a type, so that the team can
// decide whether it carries user data and belongs in EncryptableUserTypes().
TEST(DataTypeTest, ApprovedPlainTextUserTypes) {
  constexpr DataTypeSet kApprovedPlainTextUserTypes = {
      AUTOFILL_WALLET_DATA,
      AUTOFILL_WALLET_USAGE,
      AUTOFILL_VALUABLE,
      AUTOFILL_VALUABLE_METADATA,
      HISTORY_DELETE_DIRECTIVES,
      SUPERVISED_USER_SETTINGS,
      USER_EVENTS,
      USER_CONSENTS,
      SECURITY_EVENTS,
      OS_PRIORITY_PREFERENCES,
      SHARING_MESSAGE,
      INCOMING_PASSWORD_SHARING_INVITATION,
      OUTGOING_PASSWORD_SHARING_INVITATION,
      SHARED_TAB_GROUP_DATA,
      COLLABORATION_GROUP,
      SHARED_COMMENT,
      ACCOUNT_SETTING,
      SKILL,
      ENCRYPTED_TAB_CONTEXT_ITEM,
      NOTEBOOK,
      JOURNEY,
  };
  const DataTypeSet plain_text_user_types =
      base::Difference(UserTypes(), EncryptableUserTypes());
  const DataTypeSet unapproved =
      base::Difference(plain_text_user_types, kApprovedPlainTextUserTypes);
  const DataTypeSet no_longer_plain_text =
      base::Difference(kApprovedPlainTextUserTypes, plain_text_user_types);
  EXPECT_TRUE(unapproved.empty())
      << DataTypeSetToDebugString(unapproved)
      << " is not encrypted and not approved. Add it to "
         "EncryptableUserTypes() if it carries user data, otherwise approve it "
         "above.";
  EXPECT_TRUE(no_longer_plain_text.empty())
      << DataTypeSetToDebugString(no_longer_plain_text)
      << " is no longer plain text, drop it from the list above.";
}

TEST(DataTypeTest, AIChatConversationEncryptable) {
  EXPECT_TRUE(EncryptableUserTypes().Has(AI_CHAT_CONVERSATION));
}

}  // namespace
}  // namespace syncer
