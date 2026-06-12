// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_bridge.h"

#include <memory>
#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/metadata_change_list.h"
#include "components/sync/protocol/entity_data.h"
#include "components/sync/test/mock_data_type_local_change_processor.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {
namespace {

using testing::_;

class AIChatSyncBridgeTest : public testing::Test {
 public:
  void SetUp() override {
    CHECK(temp_directory_.CreateUniqueTempDir());
    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);

    base::test::TestFuture<os_crypt_async::Encryptor> future;
    os_crypt_->GetInstance(future.GetCallback());
    db_ = std::make_unique<AIChatDatabase>(db_file_path(), future.Take());
  }

  void TearDown() override {
    mock_processor_ = nullptr;
    bridge_.reset();
    db_.reset();
    CHECK(temp_directory_.Delete());
  }

  void CreateBridge() {
    auto processor = std::make_unique<
        testing::NiceMock<syncer::MockDataTypeLocalChangeProcessor>>();
    mock_processor_ = processor.get();
    ON_CALL(*mock_processor_, IsTrackingMetadata())
        .WillByDefault(testing::Return(true));
    bridge_ = std::make_unique<AIChatSyncBridge>(
        std::move(processor), db_.get(),
        base::BindRepeating([](int* counter) { ++*counter; },
                            base::Unretained(&remote_changes_applied_count_)));
  }

  int remote_changes_applied_count_ = 0;

  // Adds a conversation with one entry to the database. Returns the entry
  // UUID for convenience.
  std::string AddTestConversation(const std::string& conv_uuid,
                                  const std::string& title) {
    auto conv = mojom::Conversation::New();
    conv->uuid = conv_uuid;
    conv->title = title;
    auto entry = mojom::ConversationTurn::New();
    const std::string entry_uuid = conv_uuid + "-entry-1";
    entry->uuid = entry_uuid;
    entry->character_type = mojom::CharacterType::HUMAN;
    entry->action_type = mojom::ActionType::QUERY;
    entry->text = "Hello";
    entry->created_time = base::Time::Now();
    db_->AddConversation(std::move(conv), {}, std::move(entry));
    return entry_uuid;
  }

  base::FilePath db_file_path() {
    return temp_directory_.GetPath().AppendASCII("test_ai_chat.db");
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_directory_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  std::unique_ptr<AIChatDatabase> db_;
  std::unique_ptr<AIChatSyncBridge> bridge_;
  raw_ptr<syncer::MockDataTypeLocalChangeProcessor> mock_processor_ = nullptr;
};

TEST_F(AIChatSyncBridgeTest, InitializationCallsModelReadyToSync) {
  auto processor = std::make_unique<
      testing::NiceMock<syncer::MockDataTypeLocalChangeProcessor>>();
  EXPECT_CALL(*processor, ModelReadyToSync(_)).Times(1);
  bridge_ = std::make_unique<AIChatSyncBridge>(std::move(processor), db_.get(),
                                               base::DoNothing());
}

TEST_F(AIChatSyncBridgeTest, GetStorageKeyAndClientTagForConversation) {
  CreateBridge();
  syncer::EntityData entity_data;
  entity_data.specifics.mutable_ai_chat_conversation()
      ->mutable_conversation()
      ->set_uuid("conv-id");

  EXPECT_EQ(bridge_->GetStorageKey(entity_data), "c:conv-id");
  EXPECT_EQ(bridge_->GetClientTag(entity_data), "c:conv-id");
}

TEST_F(AIChatSyncBridgeTest, GetStorageKeyAndClientTagForEntry) {
  CreateBridge();
  syncer::EntityData entity_data;
  auto* entry =
      entity_data.specifics.mutable_ai_chat_conversation()->mutable_entry();
  entry->set_uuid("entry-id");
  entry->set_conversation_uuid("conv-id");

  EXPECT_EQ(bridge_->GetStorageKey(entity_data), "e:entry-id");
  EXPECT_EQ(bridge_->GetClientTag(entity_data), "e:entry-id");
}

TEST_F(AIChatSyncBridgeTest, IsEntityDataValid) {
  CreateBridge();

  syncer::EntityData valid_conv;
  valid_conv.specifics.mutable_ai_chat_conversation()
      ->mutable_conversation()
      ->set_uuid("has-uuid");
  EXPECT_TRUE(bridge_->IsEntityDataValid(valid_conv));

  syncer::EntityData valid_entry;
  auto* entry =
      valid_entry.specifics.mutable_ai_chat_conversation()->mutable_entry();
  entry->set_uuid("entry-uuid");
  entry->set_conversation_uuid("parent-uuid");
  EXPECT_TRUE(bridge_->IsEntityDataValid(valid_entry));

  syncer::EntityData entry_missing_parent;
  entry_missing_parent.specifics.mutable_ai_chat_conversation()
      ->mutable_entry()
      ->set_uuid("entry-uuid");
  EXPECT_FALSE(bridge_->IsEntityDataValid(entry_missing_parent));

  syncer::EntityData empty_uuid;
  empty_uuid.specifics.mutable_ai_chat_conversation()
      ->mutable_conversation()
      ->set_uuid("");
  EXPECT_FALSE(bridge_->IsEntityDataValid(empty_uuid));

  syncer::EntityData no_kind;
  no_kind.specifics.mutable_ai_chat_conversation();
  EXPECT_FALSE(bridge_->IsEntityDataValid(no_kind));

  syncer::EntityData no_specifics;
  EXPECT_FALSE(bridge_->IsEntityDataValid(no_specifics));
}

TEST_F(AIChatSyncBridgeTest,
       MergeFullSyncDataUploadsLocalConversationAndEntries) {
  const std::string entry_uuid =
      AddTestConversation("local-conv-1", "Local Conversation");
  CreateBridge();

  EXPECT_CALL(*mock_processor_, Put("c:local-conv-1", _, _)).Times(1);
  EXPECT_CALL(*mock_processor_, Put("e:" + entry_uuid, _, _)).Times(1);

  syncer::EntityChangeList empty_remote;
  bridge_->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                             std::move(empty_remote));
}

TEST_F(AIChatSyncBridgeTest, ApplyIncrementalSyncChangesDeleteConversation) {
  AddTestConversation("to-delete", "Will Be Deleted");
  CreateBridge();

  ASSERT_FALSE(db_->GetAllConversations().empty());

  syncer::EntityChangeList changes;
  syncer::EntityData delete_data;
  delete_data.specifics.mutable_ai_chat_conversation()
      ->mutable_conversation()
      ->set_uuid("to-delete");
  changes.push_back(syncer::EntityChange::CreateDelete("c:to-delete",
                                                       std::move(delete_data)));

  bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                       std::move(changes));

  for (const auto& conv : db_->GetAllConversations()) {
    EXPECT_NE(conv->uuid, "to-delete");
  }
}

TEST_F(AIChatSyncBridgeTest, ApplyIncrementalSyncChangesDeleteEntry) {
  const std::string entry_uuid = AddTestConversation("conv-with-entry", "Test");
  CreateBridge();

  // Confirm the entry exists.
  auto archive = db_->GetConversationData("conv-with-entry");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);

  syncer::EntityChangeList changes;
  syncer::EntityData delete_data;
  auto* entry =
      delete_data.specifics.mutable_ai_chat_conversation()->mutable_entry();
  entry->set_uuid(entry_uuid);
  entry->set_conversation_uuid("conv-with-entry");
  changes.push_back(syncer::EntityChange::CreateDelete("e:" + entry_uuid,
                                                       std::move(delete_data)));

  bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                       std::move(changes));

  archive = db_->GetConversationData("conv-with-entry");
  ASSERT_TRUE(archive);
  EXPECT_EQ(archive->entries.size(), 0u);
}

TEST_F(AIChatSyncBridgeTest,
       OnConversationDeletedEmitsDeleteForConvAndEntries) {
  const std::string entry_uuid = AddTestConversation("conv-to-delete", "Test");
  CreateBridge();

  EXPECT_CALL(*mock_processor_, Delete("e:" + entry_uuid, _, _)).Times(1);
  EXPECT_CALL(*mock_processor_, Delete("c:conv-to-delete", _, _)).Times(1);
  bridge_->OnConversationDeleted("conv-to-delete");
}

TEST_F(AIChatSyncBridgeTest, OnConversationAddedEmitsConversationOnly) {
  AddTestConversation("conv-to-add", "New Conv");
  CreateBridge();

  EXPECT_CALL(*mock_processor_, Put("c:conv-to-add", _, _)).Times(1);
  bridge_->OnConversationAdded("conv-to-add");
}

TEST_F(AIChatSyncBridgeTest, OnConversationEntryAddedEmitsEntryOnly) {
  const std::string entry_uuid = AddTestConversation("conv-with-entry", "Test");
  CreateBridge();

  EXPECT_CALL(*mock_processor_, Put("e:" + entry_uuid, _, _)).Times(1);
  bridge_->OnConversationEntryAdded("conv-with-entry", entry_uuid);
}

TEST_F(AIChatSyncBridgeTest, OnConversationEntryDeletedEmitsDelete) {
  CreateBridge();

  EXPECT_CALL(*mock_processor_, Delete("e:some-entry", _, _)).Times(1);
  bridge_->OnConversationEntryDeleted("some-entry");
}

TEST_F(AIChatSyncBridgeTest, OnConversationAddedNoOpWhenNotTracking) {
  AddTestConversation("conv-1", "Test");

  auto processor = std::make_unique<
      testing::NiceMock<syncer::MockDataTypeLocalChangeProcessor>>();
  mock_processor_ = processor.get();
  ON_CALL(*mock_processor_, IsTrackingMetadata())
      .WillByDefault(testing::Return(false));
  bridge_ = std::make_unique<AIChatSyncBridge>(std::move(processor), db_.get(),
                                               base::DoNothing());

  EXPECT_CALL(*mock_processor_, Put(_, _, _)).Times(0);
  bridge_->OnConversationAdded("conv-1");
}

TEST_F(AIChatSyncBridgeTest, ApplyDisableSyncChangesClearsMetadata) {
  CreateBridge();

  // Write some metadata.
  sync_pb::EntityMetadata metadata;
  metadata.set_creation_time(12345);
  db_->UpdateEntityMetadata(syncer::AI_CHAT_CONVERSATION, "test-key", metadata);

  syncer::MetadataBatch batch;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&batch));
  EXPECT_FALSE(batch.GetAllMetadata().empty());

  bridge_->ApplyDisableSyncChanges(bridge_->CreateMetadataChangeList());

  syncer::MetadataBatch batch_after;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&batch_after));
  EXPECT_TRUE(batch_after.GetAllMetadata().empty());
}

TEST_F(AIChatSyncBridgeTest, GetAllDataForDebugging) {
  AddTestConversation("debug-conv-1", "Debug 1");
  AddTestConversation("debug-conv-2", "Debug 2");
  CreateBridge();

  auto batch = bridge_->GetAllDataForDebugging();
  ASSERT_NE(batch, nullptr);

  // Each conversation contributes one Conversation record + one Entry
  // record (the seed entry added by AddTestConversation).
  int count = 0;
  while (batch->HasNext()) {
    batch->Next();
    count++;
  }
  EXPECT_EQ(count, 4);
}

TEST_F(AIChatSyncBridgeTest, GetDataForCommitConversation) {
  AddTestConversation("commit-conv", "Commit Test");
  CreateBridge();

  auto batch = bridge_->GetDataForCommit({"c:commit-conv"});
  ASSERT_NE(batch, nullptr);
  ASSERT_TRUE(batch->HasNext());

  auto [key, data] = batch->Next();
  EXPECT_EQ(key, "c:commit-conv");
  ASSERT_TRUE(data->specifics.has_ai_chat_conversation());
  ASSERT_TRUE(data->specifics.ai_chat_conversation().has_conversation());
  EXPECT_EQ(data->specifics.ai_chat_conversation().conversation().uuid(),
            "commit-conv");
}

TEST_F(AIChatSyncBridgeTest, GetDataForCommitEntry) {
  const std::string entry_uuid =
      AddTestConversation("commit-conv-2", "Commit Entry Test");
  CreateBridge();

  auto batch = bridge_->GetDataForCommit({"e:" + entry_uuid});
  ASSERT_NE(batch, nullptr);
  ASSERT_TRUE(batch->HasNext());

  auto [key, data] = batch->Next();
  EXPECT_EQ(key, "e:" + entry_uuid);
  ASSERT_TRUE(data->specifics.has_ai_chat_conversation());
  ASSERT_TRUE(data->specifics.ai_chat_conversation().has_entry());
  EXPECT_EQ(data->specifics.ai_chat_conversation().entry().uuid(), entry_uuid);
  EXPECT_EQ(data->specifics.ai_chat_conversation().entry().conversation_uuid(),
            "commit-conv-2");
}

TEST_F(AIChatSyncBridgeTest, ApplyIncrementalSyncChangesUpsertConversation) {
  CreateBridge();

  syncer::EntityChangeList changes;
  syncer::EntityData data;
  auto* conv =
      data.specifics.mutable_ai_chat_conversation()->mutable_conversation();
  conv->set_uuid("remote-conv");
  conv->set_title("From remote");
  conv->set_total_tokens(100);
  conv->set_trimmed_tokens(10);
  changes.push_back(
      syncer::EntityChange::CreateAdd("c:remote-conv", std::move(data)));

  bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                       std::move(changes));

  bool found = false;
  for (const auto& c : db_->GetAllConversations()) {
    if (c->uuid == "remote-conv") {
      found = true;
      EXPECT_EQ(c->title, "From remote");
      EXPECT_EQ(c->total_tokens, 100u);
      EXPECT_EQ(c->trimmed_tokens, 10u);
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(AIChatSyncBridgeTest,
       ApplyIncrementalSyncChangesUpsertEntryCreatesStub) {
  CreateBridge();

  // Send an entry whose parent conversation does not exist locally yet;
  // ApplyRemoteEntry should create a stub conversation row.
  syncer::EntityChangeList changes;
  syncer::EntityData data;
  auto* entry = data.specifics.mutable_ai_chat_conversation()->mutable_entry();
  entry->set_uuid("remote-entry");
  entry->set_conversation_uuid("remote-conv-stub");
  entry->set_entry_text("Hi from remote");
  entry->set_character_type(static_cast<int>(mojom::CharacterType::HUMAN));
  entry->set_action_type(static_cast<int>(mojom::ActionType::QUERY));
  entry->set_date_unix_epoch_micros(
      base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds());
  changes.push_back(
      syncer::EntityChange::CreateAdd("e:remote-entry", std::move(data)));

  bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                       std::move(changes));

  auto archive = db_->GetConversationData("remote-conv-stub");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  ASSERT_TRUE(archive->entries[0]->uuid.has_value());
  EXPECT_EQ(*archive->entries[0]->uuid, "remote-entry");
  EXPECT_EQ(archive->entries[0]->text, "Hi from remote");
}

// When an entry arrives before its parent conversation a stub row is
// created. A later metadata record must upgrade the stub in place without
// dropping the entry rows that were attached during the stub phase. This
// relies on `PRAGMA foreign_keys = OFF` so `INSERT OR REPLACE` on the
// parent does not cascade-delete its children.
TEST_F(AIChatSyncBridgeTest,
       ApplyIncrementalSyncChangesUpgradesStubConversation) {
  CreateBridge();

  // Step 1: deliver an entry whose parent does not exist locally.
  {
    syncer::EntityChangeList changes;
    syncer::EntityData data;
    auto* entry =
        data.specifics.mutable_ai_chat_conversation()->mutable_entry();
    entry->set_uuid("orphan-entry");
    entry->set_conversation_uuid("conv-to-upgrade");
    entry->set_entry_text("Pre-metadata entry");
    entry->set_character_type(static_cast<int>(mojom::CharacterType::HUMAN));
    entry->set_action_type(static_cast<int>(mojom::ActionType::QUERY));
    entry->set_date_unix_epoch_micros(
        base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds());
    changes.push_back(
        syncer::EntityChange::CreateAdd("e:orphan-entry", std::move(data)));
    bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                         std::move(changes));
  }

  // Stub exists with no title yet, entry is attached.
  auto stub = db_->GetConversationData("conv-to-upgrade");
  ASSERT_TRUE(stub);
  ASSERT_EQ(stub->entries.size(), 1u);

  // Step 2: deliver the conversation metadata. Stub should be upgraded with
  // real title/tokens, entry must still be present.
  {
    syncer::EntityChangeList changes;
    syncer::EntityData data;
    auto* conv =
        data.specifics.mutable_ai_chat_conversation()->mutable_conversation();
    conv->set_uuid("conv-to-upgrade");
    conv->set_title("Real Title");
    conv->set_total_tokens(123);
    conv->set_trimmed_tokens(45);
    changes.push_back(
        syncer::EntityChange::CreateAdd("c:conv-to-upgrade", std::move(data)));
    bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                         std::move(changes));
  }

  // Title is populated and the entry survived the REPLACE.
  bool found = false;
  for (const auto& c : db_->GetAllConversations()) {
    if (c->uuid == "conv-to-upgrade") {
      found = true;
      EXPECT_EQ(c->title, "Real Title");
      EXPECT_EQ(c->total_tokens, 123u);
      EXPECT_EQ(c->trimmed_tokens, 45u);
    }
  }
  EXPECT_TRUE(found);

  auto upgraded = db_->GetConversationData("conv-to-upgrade");
  ASSERT_TRUE(upgraded);
  ASSERT_EQ(upgraded->entries.size(), 1u);
  ASSERT_TRUE(upgraded->entries[0]->uuid.has_value());
  EXPECT_EQ(*upgraded->entries[0]->uuid, "orphan-entry");
  EXPECT_EQ(upgraded->entries[0]->text, "Pre-metadata entry");
}

// A delete tombstone for an entry that does not exist locally must be a
// silent no-op (the local store may simply never have seen the entry).
TEST_F(AIChatSyncBridgeTest,
       ApplyIncrementalSyncChangesDeleteUnknownEntryIsNoOp) {
  AddTestConversation("conv-with-known-entry", "Test");
  CreateBridge();

  // Snapshot the known-good state.
  auto before = db_->GetConversationData("conv-with-known-entry");
  ASSERT_TRUE(before);
  ASSERT_EQ(before->entries.size(), 1u);

  syncer::EntityChangeList changes;
  syncer::EntityData delete_data;
  auto* entry =
      delete_data.specifics.mutable_ai_chat_conversation()->mutable_entry();
  entry->set_uuid("never-existed");
  entry->set_conversation_uuid("conv-with-known-entry");
  changes.push_back(syncer::EntityChange::CreateDelete("e:never-existed",
                                                       std::move(delete_data)));

  auto error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(), std::move(changes));
  EXPECT_FALSE(error.has_value());

  // The unrelated existing entry and conversation must be untouched.
  auto after = db_->GetConversationData("conv-with-known-entry");
  ASSERT_TRUE(after);
  EXPECT_EQ(after->entries.size(), 1u);
}

// When the remote sender marks AC last_contents as truncated, the bridge
// must substitute the local text rather than overwriting it with empty.
// Regression: previously ApplyRemoteEntry passed an empty-strings vector
// unconditionally, wiping locally-present page content on every apply.
TEST_F(AIChatSyncBridgeTest,
       ApplyRemoteEntryWithTruncatedLastContentsPreservesLocal) {
  // Seed local DB with a conversation entry that has AC + content text.
  auto conv = mojom::Conversation::New();
  conv->uuid = "conv-preserve";
  conv->title = "Local";
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-preserve";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->text = "Question?";
  entry->created_time = base::Time::Now();
  ASSERT_TRUE(db_->AddConversation(std::move(conv), {}, std::move(entry)));

  std::vector<mojom::AssociatedContentPtr> acs;
  acs.push_back(mojom::AssociatedContent::New(
      "ac-preserve", mojom::ContentType::PageContent, "Local page", 0,
      GURL("https://example.com/local"), 80,
      std::optional<std::string>("entry-preserve")));
  std::vector<std::string> contents = {"ORIGINAL local page text"};
  ASSERT_TRUE(db_->AddOrUpdateAssociatedContent("conv-preserve", std::move(acs),
                                                std::move(contents)));

  // Sanity-check the seed.
  {
    auto seeded = db_->GetConversationData("conv-preserve");
    ASSERT_TRUE(seeded);
    ASSERT_EQ(seeded->associated_content.size(), 1u);
    ASSERT_EQ(seeded->associated_content[0]->content,
              "ORIGINAL local page text");
  }

  CreateBridge();

  // Build a remote update with the AC last_contents marked truncated.
  syncer::EntityChangeList changes;
  syncer::EntityData data;
  auto* remote_entry =
      data.specifics.mutable_ai_chat_conversation()->mutable_entry();
  remote_entry->set_uuid("entry-preserve");
  remote_entry->set_conversation_uuid("conv-preserve");
  remote_entry->set_entry_text("Question?");
  remote_entry->set_character_type(
      static_cast<int>(mojom::CharacterType::HUMAN));
  remote_entry->set_action_type(static_cast<int>(mojom::ActionType::QUERY));
  remote_entry->set_date_unix_epoch_micros(
      base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds());
  auto* remote_ac = remote_entry->add_associated_content();
  remote_ac->set_uuid("ac-preserve");
  remote_ac->set_title("Local page");
  remote_ac->set_url("https://example.com/local");
  remote_ac->set_content_type(
      static_cast<int>(mojom::ContentType::PageContent));
  remote_ac->set_content_used_percentage(80);
  MarkCompressibleStringTruncated(remote_ac->mutable_last_contents());

  changes.push_back(
      syncer::EntityChange::CreateAdd("e:entry-preserve", std::move(data)));
  bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                       std::move(changes));

  // The local AC's content text must be preserved.
  auto archive = db_->GetConversationData("conv-preserve");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->associated_content.size(), 1u);
  EXPECT_EQ(archive->associated_content[0]->content_uuid, "ac-preserve");
  EXPECT_EQ(archive->associated_content[0]->content,
            "ORIGINAL local page text");
}

// Whenever a Merge/Apply batch actually changes the local DB, the bridge
// must invoke the on-remote-changes-applied closure exactly once so the
// service can refresh in-memory state and active conversation handlers.
TEST_F(AIChatSyncBridgeTest, NotifiesRemoteChangesAppliedOnAddBatch) {
  CreateBridge();
  ASSERT_EQ(remote_changes_applied_count_, 0);

  syncer::EntityChangeList changes;
  syncer::EntityData data;
  data.specifics.mutable_ai_chat_conversation()
      ->mutable_conversation()
      ->set_uuid("remote-conv");
  changes.push_back(
      syncer::EntityChange::CreateAdd("c:remote-conv", std::move(data)));
  bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                       std::move(changes));

  EXPECT_EQ(remote_changes_applied_count_, 1);
}

TEST_F(AIChatSyncBridgeTest, DoesNotNotifyWhenNoRemoteChangesApplied) {
  CreateBridge();

  syncer::EntityChangeList empty_changes;
  bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                       std::move(empty_changes));

  EXPECT_EQ(remote_changes_applied_count_, 0);
}

}  // namespace
}  // namespace ai_chat
