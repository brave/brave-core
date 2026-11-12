/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/engine/brave_commit_encryption_check.h"

#include <memory>
#include <utility>

#include "components/sync/protocol/entity_data.h"
#include "components/sync/protocol/entity_specifics.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

std::unique_ptr<CommitRequestData> MakeCommitRequest(
    sync_pb::EntitySpecifics specifics) {
  auto entity = std::make_unique<EntityData>();
  entity->specifics = std::move(specifics);
  auto commit_request = std::make_unique<CommitRequestData>();
  commit_request->entity = std::move(entity);
  return commit_request;
}

sync_pb::EntitySpecifics EncryptedSpecifics() {
  sync_pb::EntitySpecifics specifics;
  specifics.mutable_encrypted()->set_blob("blob");
  return specifics;
}

sync_pb::EntitySpecifics BookmarkSpecifics() {
  sync_pb::EntitySpecifics specifics;
  specifics.mutable_bookmark();
  return specifics;
}

}  // namespace

TEST(BraveCommitEncryptionCheckTest, EncryptedEntityIsAccepted) {
  EntityData entity;
  entity.specifics = EncryptedSpecifics();
  EXPECT_FALSE(IsUnexpectedPlainTextCommit(BOOKMARKS, entity));
}

TEST(BraveCommitEncryptionCheckTest, DeletionIsAccepted) {
  EntityData entity;
  ASSERT_TRUE(entity.is_deleted());
  EXPECT_FALSE(IsUnexpectedPlainTextCommit(BOOKMARKS, entity));
}

TEST(BraveCommitEncryptionCheckTest, PlainTextAllowedTypes) {
  EntityData entity;
  entity.specifics.mutable_nigori();
  EXPECT_FALSE(IsUnexpectedPlainTextCommit(NIGORI, entity));
  entity.specifics.mutable_history_delete_directive();
  EXPECT_FALSE(IsUnexpectedPlainTextCommit(HISTORY_DELETE_DIRECTIVES, entity));
  entity.specifics.mutable_password();
  EXPECT_FALSE(IsUnexpectedPlainTextCommit(PASSWORDS, entity));
}

TEST(BraveCommitEncryptionCheckTest, PlainTextBookmarkIsReported) {
  EntityData entity;
  entity.specifics = BookmarkSpecifics();
  EXPECT_TRUE(IsUnexpectedPlainTextCommit(BOOKMARKS, entity));
}

TEST(BraveCommitEncryptionCheckTest, PlainTextContactInfoIsReported) {
  EntityData entity;
  entity.specifics.mutable_contact_info();
  EXPECT_TRUE(IsUnexpectedPlainTextCommit(CONTACT_INFO, entity));
}

TEST(BraveCommitEncryptionCheckTest, KeystorePassphraseDropsNothing) {
  CommitRequestDataList commit_requests;
  commit_requests.push_back(MakeCommitRequest(BookmarkSpecifics()));
  EXPECT_EQ(
      DropUnencryptedCommitRequests(
          BOOKMARKS, PassphraseType::kKeystorePassphrase, &commit_requests),
      0u);
  EXPECT_EQ(commit_requests.size(), 1u);
}

TEST(BraveCommitEncryptionCheckTest, EncryptedCommitRequestsAreKept) {
  CommitRequestDataList commit_requests;
  commit_requests.push_back(MakeCommitRequest(EncryptedSpecifics()));
  commit_requests.push_back(MakeCommitRequest(EncryptedSpecifics()));
  EXPECT_EQ(DropUnencryptedCommitRequests(
                BOOKMARKS, PassphraseType::kCustomPassphrase, &commit_requests),
            0u);
  EXPECT_EQ(commit_requests.size(), 2u);
}

TEST(BraveCommitEncryptionCheckTest, PlainTextAllowedTypeIsKept) {
  sync_pb::EntitySpecifics specifics;
  specifics.mutable_password();
  CommitRequestDataList commit_requests;
  commit_requests.push_back(MakeCommitRequest(std::move(specifics)));
  EXPECT_EQ(DropUnencryptedCommitRequests(
                PASSWORDS, PassphraseType::kCustomPassphrase, &commit_requests),
            0u);
  EXPECT_EQ(commit_requests.size(), 1u);
}

TEST(BraveCommitEncryptionCheckTest, PlainTextCommitRequestIsDropped) {
  CommitRequestDataList commit_requests;
  commit_requests.push_back(MakeCommitRequest(EncryptedSpecifics()));
  commit_requests.push_back(MakeCommitRequest(BookmarkSpecifics()));
  commit_requests.push_back(MakeCommitRequest(EncryptedSpecifics()));
  EXPECT_EQ(DropUnencryptedCommitRequests(
                BOOKMARKS, PassphraseType::kCustomPassphrase, &commit_requests),
            1u);
  ASSERT_EQ(commit_requests.size(), 2u);
  EXPECT_TRUE(commit_requests[0]->entity->specifics.has_encrypted());
  EXPECT_TRUE(commit_requests[1]->entity->specifics.has_encrypted());
}

// Leaves GetContribution() with an empty list, which is what makes it return
// nullptr instead of committing.
TEST(BraveCommitEncryptionCheckTest, AllPlainTextCommitRequestsAreDropped) {
  CommitRequestDataList commit_requests;
  commit_requests.push_back(MakeCommitRequest(BookmarkSpecifics()));
  commit_requests.push_back(MakeCommitRequest(BookmarkSpecifics()));
  EXPECT_EQ(DropUnencryptedCommitRequests(
                BOOKMARKS, PassphraseType::kCustomPassphrase, &commit_requests),
            2u);
  EXPECT_TRUE(commit_requests.empty());
}

}  // namespace syncer
