/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/engine/brave_commit_encryption_check.h"

#include "components/sync/protocol/entity_specifics.pb.h"
#include "components/sync/protocol/sync_entity.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

TEST(BraveCommitEncryptionCheckTest, EncryptedEntityIsAccepted) {
  sync_pb::SyncEntity entity;
  entity.mutable_specifics()->mutable_encrypted()->set_blob("blob");
  EXPECT_FALSE(IsUnexpectedPlainTextCommit(BOOKMARKS, entity));
}

TEST(BraveCommitEncryptionCheckTest, DeletionIsAccepted) {
  sync_pb::SyncEntity entity;
  entity.set_deleted(true);
  entity.mutable_specifics()->mutable_bookmark();
  EXPECT_FALSE(IsUnexpectedPlainTextCommit(BOOKMARKS, entity));
}

TEST(BraveCommitEncryptionCheckTest, PlainTextAllowedTypes) {
  sync_pb::SyncEntity entity;
  entity.mutable_specifics()->mutable_nigori();
  EXPECT_FALSE(IsUnexpectedPlainTextCommit(NIGORI, entity));
  entity.mutable_specifics()->mutable_history_delete_directive();
  EXPECT_FALSE(IsUnexpectedPlainTextCommit(HISTORY_DELETE_DIRECTIVES, entity));
  entity.mutable_specifics()->mutable_password();
  EXPECT_FALSE(IsUnexpectedPlainTextCommit(PASSWORDS, entity));
}

TEST(BraveCommitEncryptionCheckTest, PlainTextBookmarkIsReported) {
  sync_pb::SyncEntity entity;
  entity.mutable_specifics()->mutable_bookmark();
  EXPECT_TRUE(IsUnexpectedPlainTextCommit(BOOKMARKS, entity));
}

TEST(BraveCommitEncryptionCheckTest, PlainTextContactInfoIsReported) {
  sync_pb::SyncEntity entity;
  entity.mutable_specifics()->mutable_contact_info();
  EXPECT_TRUE(IsUnexpectedPlainTextCommit(CONTACT_INFO, entity));
}

TEST(BraveCommitEncryptionCheckTest, KeystorePassphraseIsNotChecked) {
  sync_pb::SyncEntity entity;
  entity.mutable_specifics()->mutable_bookmark();
  // Would DCHECK with an explicit passphrase.
  CheckCommitEntityEncryption(BOOKMARKS, PassphraseType::kKeystorePassphrase,
                              entity);
}

}  // namespace syncer
