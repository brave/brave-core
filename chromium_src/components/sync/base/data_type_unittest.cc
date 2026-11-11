/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/containers/enum_set.h"

#include <components/sync/base/data_type_unittest.cc>

namespace syncer {

// Defined by the override in chromium_src/components/sync/base/data_type.cc,
// which has no header to declare it.
DataTypeSet EncryptableUserTypes_ChromiumImpl();

namespace {

TEST(DataTypeTest, EncryptableUserTypes) {
  EXPECT_TRUE(EncryptableUserTypes().Has(CONTACT_INFO));
  EXPECT_TRUE(EncryptableUserTypes().Has(DEVICE_INFO));
  EXPECT_TRUE(EncryptableUserTypes().Has(HISTORY));
  EXPECT_TRUE(EncryptableUserTypes().Has(PRIORITY_PREFERENCES));
  // Carries tab urls and titles. Upstream keeps it plain so that collaborators
  // and the server can read it, which Brave has no infrastructure for, so it
  // fails closed here instead.
  EXPECT_TRUE(EncryptableUserTypes().Has(SHARED_TAB_GROUP_DATA));
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
// outside EncryptableUserTypes() reaches the server as plain text. The override
// subtracts, so this list has to be spelled out rather than derived from the
// same sets: that way a Chromium bump adding a commit-only type still fails
// here, and the team can decide whether it carries user data.
TEST(DataTypeTest, ApprovedPlainTextUserTypes) {
  constexpr DataTypeSet kApprovedPlainTextUserTypes = {
      HISTORY_DELETE_DIRECTIVES,
      USER_EVENTS,
      USER_CONSENTS,
      SECURITY_EVENTS,
      SHARING_MESSAGE,
      OUTGOING_PASSWORD_SHARING_INVITATION,
      ENCRYPTED_TAB_CONTEXT_ITEM,
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
         "EncryptableUserTypes() if it carries user data, otherwise add it to "
         "kApprovedPlainTextUserTypes.";
  EXPECT_TRUE(no_longer_plain_text.empty())
      << DataTypeSetToDebugString(no_longer_plain_text)
      << " is no longer plain text, drop it from "
         "kApprovedPlainTextUserTypes.";
}

// The override subtracts and so ignores EncryptionPolicy entirely, which means
// a bump that flips a type's policy changes nothing in EncryptableUserTypes()
// and would pass ApprovedPlainTextUserTypes unnoticed. Pin upstream's own view
// of it here, so the flip surfaces as a failure and the team can judge whether
// the new rationale holds for Brave's server. Expect this to fail alongside
// DataTypeCounts on every type addition.
TEST(DataTypeTest, ChromiumPlainTextUserTypes) {
  constexpr DataTypeSet kChromiumPlainTextUserTypes = {
      AUTOFILL_WALLET_DATA,
      AUTOFILL_WALLET_USAGE,
      HISTORY_DELETE_DIRECTIVES,
      DEVICE_INFO,
      PRIORITY_PREFERENCES,
      SUPERVISED_USER_SETTINGS,
      USER_EVENTS,
      USER_CONSENTS,
      SECURITY_EVENTS,
      OS_PRIORITY_PREFERENCES,
      SHARING_MESSAGE,
      HISTORY,
      CONTACT_INFO,
      INCOMING_PASSWORD_SHARING_INVITATION,
      OUTGOING_PASSWORD_SHARING_INVITATION,
      SHARED_TAB_GROUP_DATA,
      COLLABORATION_GROUP,
      AUTOFILL_VALUABLE,
      AUTOFILL_VALUABLE_METADATA,
      SHARED_COMMENT,
      ACCOUNT_SETTING,
      SKILL,
      ENCRYPTED_TAB_CONTEXT_ITEM,
      JOURNEY,
      NOTEBOOK,
  };
  const DataTypeSet chromium_plain_text_user_types =
      base::Difference(UserTypes(), EncryptableUserTypes_ChromiumImpl());
  const DataTypeSet newly_plain_text = base::Difference(
      chromium_plain_text_user_types, kChromiumPlainTextUserTypes);
  const DataTypeSet newly_encryptable = base::Difference(
      kChromiumPlainTextUserTypes, chromium_plain_text_user_types);
  EXPECT_TRUE(newly_plain_text.empty())
      << DataTypeSetToDebugString(newly_plain_text)
      << " is now kNeverEncrypted upstream. Brave encrypts it regardless; "
         "check the new rationale and record it here.";
  EXPECT_TRUE(newly_encryptable.empty())
      << DataTypeSetToDebugString(newly_encryptable)
      << " is no longer kNeverEncrypted upstream, drop it from the list above.";
}

TEST(DataTypeTest, AIChatConversationEncryptable) {
  EXPECT_TRUE(EncryptableUserTypes().Has(AI_CHAT_CONVERSATION));
}

}  // namespace
}  // namespace syncer
