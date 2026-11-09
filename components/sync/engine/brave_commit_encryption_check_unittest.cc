/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/engine/brave_commit_encryption_check.h"

#include "components/sync/protocol/entity_specifics.pb.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/protocol/sync_entity.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

TEST(BraveCommitEncryptionCheckTest, EmptyCommit) {
  sync_pb::CommitMessage commit;
  EXPECT_TRUE(GetUnexpectedPlainTextCommitTypes(commit).empty());
}

TEST(BraveCommitEncryptionCheckTest, EncryptedEntityIsAccepted) {
  sync_pb::CommitMessage commit;
  commit.add_entries()->mutable_specifics()->mutable_encrypted()->set_blob(
      "blob");
  EXPECT_TRUE(GetUnexpectedPlainTextCommitTypes(commit).empty());
}

TEST(BraveCommitEncryptionCheckTest, DeletionIsAccepted) {
  sync_pb::CommitMessage commit;
  sync_pb::SyncEntity* entity = commit.add_entries();
  entity->set_deleted(true);
  entity->mutable_specifics()->mutable_bookmark();
  EXPECT_TRUE(GetUnexpectedPlainTextCommitTypes(commit).empty());
}

TEST(BraveCommitEncryptionCheckTest, PlainTextAllowedTypes) {
  sync_pb::CommitMessage commit;
  commit.add_entries()->mutable_specifics()->mutable_nigori();
  commit.add_entries()->mutable_specifics()->mutable_history_delete_directive();
  commit.add_entries()->mutable_specifics()->mutable_password();
  EXPECT_TRUE(GetUnexpectedPlainTextCommitTypes(commit).empty());
}

TEST(BraveCommitEncryptionCheckTest, PlainTextBookmarkIsReported) {
  sync_pb::CommitMessage commit;
  commit.add_entries()->mutable_specifics()->mutable_bookmark();
  EXPECT_EQ(DataTypeSet({BOOKMARKS}),
            GetUnexpectedPlainTextCommitTypes(commit));
}

TEST(BraveCommitEncryptionCheckTest, AllOffendingTypesAreGathered) {
  sync_pb::CommitMessage commit;
  commit.add_entries()->mutable_specifics()->mutable_bookmark();
  commit.add_entries()->mutable_specifics()->mutable_contact_info();
  commit.add_entries()->mutable_specifics()->mutable_bookmark();
  commit.add_entries()->mutable_specifics()->mutable_nigori();
  commit.add_entries()->mutable_specifics()->mutable_encrypted()->set_blob(
      "blob");
  EXPECT_EQ(DataTypeSet({BOOKMARKS, CONTACT_INFO}),
            GetUnexpectedPlainTextCommitTypes(commit));
}

TEST(BraveCommitEncryptionCheckTest, UnsetSpecificsVariantIsIgnored) {
  sync_pb::CommitMessage commit;
  commit.add_entries();
  EXPECT_TRUE(GetUnexpectedPlainTextCommitTypes(commit).empty());
}

}  // namespace syncer
