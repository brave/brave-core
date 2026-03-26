// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_bridge.h"

#include <memory>
#include <string>

#include "base/files/scoped_temp_dir.h"
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
    bridge_ =
        std::make_unique<AIChatSyncBridge>(std::move(processor), db_.get());
  }

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
  bridge_ = std::make_unique<AIChatSyncBridge>(std::move(processor), db_.get());
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
  bridge_ = std::make_unique<AIChatSyncBridge>(std::move(processor), db_.get());

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

}  // namespace
}  // namespace ai_chat
