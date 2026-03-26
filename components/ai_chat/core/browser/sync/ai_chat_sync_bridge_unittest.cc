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
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/os_crypt/sync/os_crypt_mocker.h"
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
    OSCryptMocker::SetUp();
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
    OSCryptMocker::TearDown();
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

  // Helper to add a conversation to the database directly.
  void AddTestConversation(const std::string& uuid, const std::string& title) {
    auto conv = mojom::Conversation::New();
    conv->uuid = uuid;
    conv->title = title;
    auto entry = mojom::ConversationTurn::New();
    entry->uuid = uuid + "-entry-1";
    entry->character_type = mojom::CharacterType::HUMAN;
    entry->action_type = mojom::ActionType::QUERY;
    entry->text = "Hello";
    entry->created_time = base::Time::Now();
    db_->AddConversation(std::move(conv), {}, std::move(entry));
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

TEST_F(AIChatSyncBridgeTest, GetStorageKeyAndClientTag) {
  CreateBridge();
  syncer::EntityData entity_data;
  entity_data.specifics.mutable_ai_chat_conversation()->set_uuid("test-id");

  EXPECT_EQ(bridge_->GetStorageKey(entity_data), "test-id");
  EXPECT_EQ(bridge_->GetClientTag(entity_data), "test-id");
}

TEST_F(AIChatSyncBridgeTest, IsEntityDataValid) {
  CreateBridge();

  syncer::EntityData valid;
  valid.specifics.mutable_ai_chat_conversation()->set_uuid("has-uuid");
  EXPECT_TRUE(bridge_->IsEntityDataValid(valid));

  syncer::EntityData empty_uuid;
  empty_uuid.specifics.mutable_ai_chat_conversation()->set_uuid("");
  EXPECT_FALSE(bridge_->IsEntityDataValid(empty_uuid));

  syncer::EntityData no_specifics;
  EXPECT_FALSE(bridge_->IsEntityDataValid(no_specifics));
}

TEST_F(AIChatSyncBridgeTest, MergeFullSyncDataUploadsLocalConversations) {
  AddTestConversation("local-conv-1", "Local Conversation");
  CreateBridge();

  EXPECT_CALL(*mock_processor_, Put("local-conv-1", _, _)).Times(1);

  syncer::EntityChangeList empty_remote;
  bridge_->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                             std::move(empty_remote));
}

TEST_F(AIChatSyncBridgeTest, MergeFullSyncDataAppliesRemoteConversations) {
  CreateBridge();

  syncer::EntityChangeList remote_changes;
  auto entity_data = std::make_unique<syncer::EntityData>();
  auto* specifics = entity_data->specifics.mutable_ai_chat_conversation();
  specifics->set_uuid("remote-conv-1");
  specifics->set_title("Remote Conversation");
  auto* entry = specifics->add_entries();
  entry->set_uuid("remote-entry-1");
  entry->set_entry_text("Hello from remote");
  entry->set_character_type(0);
  remote_changes.push_back(syncer::EntityChange::CreateAdd(
      "remote-conv-1", std::move(*entity_data)));

  bridge_->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                             std::move(remote_changes));

  auto conversations = db_->GetAllConversations();
  bool found = false;
  for (const auto& conv : conversations) {
    if (conv->uuid == "remote-conv-1") {
      EXPECT_EQ(conv->title, "Remote Conversation");
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found) << "Remote conversation not found in database";
}

TEST_F(AIChatSyncBridgeTest, ApplyIncrementalSyncChangesAdd) {
  CreateBridge();

  syncer::EntityChangeList changes;
  auto entity_data = std::make_unique<syncer::EntityData>();
  auto* specifics = entity_data->specifics.mutable_ai_chat_conversation();
  specifics->set_uuid("incremental-conv-1");
  specifics->set_title("Incremental Add");
  auto* entry = specifics->add_entries();
  entry->set_uuid("inc-entry-1");
  entry->set_entry_text("Test entry");
  entry->set_character_type(0);
  changes.push_back(syncer::EntityChange::CreateAdd("incremental-conv-1",
                                                    std::move(*entity_data)));

  bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                       std::move(changes));

  auto conversations = db_->GetAllConversations();
  bool found = false;
  for (const auto& conv : conversations) {
    if (conv->uuid == "incremental-conv-1") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found) << "Incrementally added conversation not in database";
}

TEST_F(AIChatSyncBridgeTest, ApplyIncrementalSyncChangesDelete) {
  AddTestConversation("to-delete", "Will Be Deleted");
  CreateBridge();

  // Verify it exists.
  ASSERT_FALSE(db_->GetAllConversations().empty());

  syncer::EntityChangeList changes;
  syncer::EntityData delete_data;
  delete_data.specifics.mutable_ai_chat_conversation()->set_uuid("to-delete");
  changes.push_back(
      syncer::EntityChange::CreateDelete("to-delete", std::move(delete_data)));

  bridge_->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                       std::move(changes));

  // Verify it was deleted.
  auto conversations = db_->GetAllConversations();
  for (const auto& conv : conversations) {
    EXPECT_NE(conv->uuid, "to-delete");
  }
}

TEST_F(AIChatSyncBridgeTest, OnConversationDeletedCallsProcessorDelete) {
  AddTestConversation("conv-to-delete", "Test");
  CreateBridge();

  EXPECT_CALL(*mock_processor_, Delete("conv-to-delete", _, _)).Times(1);
  bridge_->OnConversationDeleted("conv-to-delete");
}

TEST_F(AIChatSyncBridgeTest, OnConversationAddedCallsProcessorPut) {
  AddTestConversation("conv-to-add", "New Conv");
  CreateBridge();

  EXPECT_CALL(*mock_processor_, Put("conv-to-add", _, _)).Times(1);
  bridge_->OnConversationAdded("conv-to-add");
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

  // Verify metadata exists.
  syncer::MetadataBatch batch;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&batch));
  EXPECT_FALSE(batch.GetAllMetadata().empty());

  bridge_->ApplyDisableSyncChanges(bridge_->CreateMetadataChangeList());

  // Verify metadata was cleared.
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

  // Consume the batch and count entries.
  int count = 0;
  while (batch->HasNext()) {
    batch->Next();
    count++;
  }
  EXPECT_EQ(count, 2);
}

TEST_F(AIChatSyncBridgeTest, GetDataForCommit) {
  AddTestConversation("commit-conv", "Commit Test");
  CreateBridge();

  auto batch = bridge_->GetDataForCommit({"commit-conv"});
  ASSERT_NE(batch, nullptr);
  ASSERT_TRUE(batch->HasNext());

  auto [key, data] = batch->Next();
  EXPECT_EQ(key, "commit-conv");
  EXPECT_TRUE(data->specifics.has_ai_chat_conversation());
  EXPECT_EQ(data->specifics.ai_chat_conversation().uuid(), "commit-conv");
}

}  // namespace
}  // namespace ai_chat
