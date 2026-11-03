// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_bridge.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/metadata_change_list.h"
#include "components/sync/model/model_error.h"
#include "components/sync/protocol/data_type_state.pb.h"
#include "components/sync/protocol/entity_data.h"
#include "components/sync/protocol/entity_metadata.pb.h"
#include "components/sync/test/mock_data_type_local_change_processor.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {
namespace {

using testing::_;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::Pair;
using testing::Pointee;
using testing::Property;
using testing::UnorderedElementsAre;

// Matches the EntityData passed to Put() when its record carries |time| as its
// modified time.
MATCHER_P(HasModifiedTime, time, "") {
  return GetModifiedTimeFromSpecifics(arg->specifics.ai_chat_conversation()) ==
         time;
}

// Stands in for the processor recording a local Put() or Delete() in its
// metadata change list, which is the third argument of both.
ACTION(RecordLocalChangeMetadata) {
  sync_pb::EntityMetadata metadata;
  metadata.set_server_version(1);
  arg2->UpdateMetadata(arg0, metadata);
}

// Matches a saved EntityMetadata pointer whose record version is |version|.
auto HasServerVersion(int64_t version) {
  return Pointee(Property(&sync_pb::EntityMetadata::server_version, version));
}

// A database whose remote-apply and delete paths start failing after
// |writes_before_failure| successful writes, to exercise the bridge's error
// propagation without needing real disk failures.
class FailingAIChatDatabase : public AIChatDatabase {
 public:
  FailingAIChatDatabase(const base::FilePath& db_file_path,
                        scoped_refptr<os_crypt_async::Encryptor> encryptor,
                        int writes_before_failure)
      : AIChatDatabase(db_file_path, std::move(encryptor)),
        writes_remaining_(writes_before_failure) {}

  bool ApplyRemoteConversationMetadata(mojom::ConversationPtr conversation,
                                       base::Time modified_time) override {
    return ConsumeWrite() && AIChatDatabase::ApplyRemoteConversationMetadata(
                                 std::move(conversation), modified_time);
  }

  bool ApplyRemoteEntry(
      std::string_view conversation_uuid,
      mojom::ConversationTurnPtr entry,
      base::Time modified_time,
      std::vector<mojom::AssociatedContentPtr> associated_content,
      std::vector<std::string> contents) override {
    return ConsumeWrite() &&
           AIChatDatabase::ApplyRemoteEntry(
               conversation_uuid, std::move(entry), modified_time,
               std::move(associated_content), std::move(contents));
  }

  bool DeleteConversation(std::string_view conversation_uuid) override {
    return ConsumeWrite() &&
           AIChatDatabase::DeleteConversation(conversation_uuid);
  }

  bool DeleteConversationEntry(std::string_view entry_uuid) override {
    return ConsumeWrite() &&
           AIChatDatabase::DeleteConversationEntry(entry_uuid);
  }

 private:
  bool ConsumeWrite() { return writes_remaining_-- > 0; }

  int writes_remaining_;
};

// A conversation metadata record, as another device would send it.
syncer::EntityData MakeRemoteConversation(const std::string& uuid,
                                          const std::string& title) {
  syncer::EntityData data;
  auto* conversation =
      data.specifics.mutable_ai_chat_conversation()->mutable_conversation();
  conversation->set_uuid(uuid);
  conversation->set_title(title);
  return data;
}

// A single remote conversation add.
syncer::EntityChangeList MakeConversationAdd(const std::string& uuid) {
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateAdd(
      "c:" + uuid, MakeRemoteConversation(uuid, "Remote")));
  return changes;
}

// An entry record for a single turn, as another device would send it.
syncer::EntityData MakeRemoteEntry(const std::string& conv_uuid,
                                   const std::string& entry_uuid,
                                   const std::string& text) {
  syncer::EntityData data;
  auto* entry = data.specifics.mutable_ai_chat_conversation()->mutable_entry();
  entry->set_uuid(entry_uuid);
  entry->set_conversation_uuid(conv_uuid);
  entry->set_entry_text(text);
  entry->set_character_type(std::to_underlying(mojom::CharacterType::HUMAN));
  entry->set_action_type(std::to_underlying(mojom::ActionType::QUERY));
  entry->set_created_time_windows_epoch_micros(
      base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds());
  return data;
}

// Adds page content |content_uuid| to the remote entry record |data|.
void AddRemoteAssociatedContent(syncer::EntityData& data,
                                const std::string& content_uuid) {
  auto* content = data.specifics.mutable_ai_chat_conversation()
                      ->mutable_entry()
                      ->add_associated_content();
  content->set_uuid(content_uuid);
  content->set_title("Remote page");
  content->set_url("https://example.com/remote");
  content->set_content_type(
      std::to_underlying(mojom::ContentType::PageContent));
  content->set_content_used_percentage(100);
  content->mutable_last_contents()->set_raw("Remote page text");
}

class AIChatSyncBridgeTest : public testing::Test {
 public:
  void SetUp() override {
    CHECK(temp_directory_.CreateUniqueTempDir());
    db_ = std::make_unique<AIChatDatabase>(
        db_file_path(), os_crypt_async::GetTestEncryptorForTesting());
  }

  void TearDown() override {
    mock_processor_ = nullptr;
    bridge_.reset();
    db_.reset();
    CHECK(temp_directory_.Delete());
  }

  // Creates the bridge and attaches the database (the common case).
  void CreateBridge(bool is_tracking_metadata = true) {
    CreateBridgeWithoutDatabase(is_tracking_metadata);
    bridge_->SetDatabase(db_.get());
  }

  // Creates the bridge without attaching a database, mirroring how the service
  // installs the bridge eagerly before the database exists.
  void CreateBridgeWithoutDatabase(bool is_tracking_metadata = true) {
    auto processor = std::make_unique<
        testing::NiceMock<syncer::MockDataTypeLocalChangeProcessor>>();
    mock_processor_ = processor.get();
    ON_CALL(*mock_processor_, IsTrackingMetadata())
        .WillByDefault(testing::Return(is_tracking_metadata));
    bridge_ = std::make_unique<AIChatSyncBridge>(
        std::move(processor),
        base::BindRepeating(&AIChatSyncBridgeTest::OnRemoteChangesApplied,
                            base::Unretained(this)));
  }

  void OnRemoteChangesApplied(std::vector<std::string> conversation_uuids) {
    ++remote_changes_applied_count_;
    last_affected_conversation_uuids_ = std::move(conversation_uuids);
  }

  // Replaces the database with one that fails every remote write after
  // |writes_before_failure| of them, then creates the bridge over it.
  void CreateBridgeWithFailingDatabase(int writes_before_failure) {
    db_.reset();
    db_ = std::make_unique<FailingAIChatDatabase>(
        db_file_path(), os_crypt_async::GetTestEncryptorForTesting(),
        writes_before_failure);
    CreateBridge();
  }

  // Returns the metadata changes the processor makes before it hands a batch
  // to the bridge: the batch's progress marker, and the new version of
  // |storage_key|'s record.
  std::unique_ptr<syncer::MetadataChangeList> MakeBatchMetadataChanges(
      const std::string& progress_token,
      const std::string& storage_key,
      int64_t server_version) {
    auto metadata_change_list = bridge_->CreateMetadataChangeList();
    sync_pb::DataTypeState state;
    state.mutable_progress_marker()->set_token(progress_token);
    metadata_change_list->UpdateDataTypeState(state);
    sync_pb::EntityMetadata metadata;
    metadata.set_server_version(server_version);
    metadata_change_list->UpdateMetadata(storage_key, metadata);
    return metadata_change_list;
  }

  // Saves a progress marker and a version of |storage_key|'s record straight
  // to the database, as an earlier batch would have.
  void SaveSyncMetadata(const std::string& progress_token,
                        const std::string& storage_key,
                        int64_t server_version) {
    sync_pb::DataTypeState state;
    state.mutable_progress_marker()->set_token(progress_token);
    ASSERT_TRUE(db_->UpdateDataTypeState(syncer::AI_CHAT_CONVERSATION, state));
    sync_pb::EntityMetadata metadata;
    metadata.set_server_version(server_version);
    ASSERT_TRUE(db_->UpdateEntityMetadata(syncer::AI_CHAT_CONVERSATION,
                                          storage_key, metadata));
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

  // Adds a conversation whose single entry is oversized by a field the size
  // budget cannot omit, so it can never be committed, and gives the entry
  // persisted sync metadata.
  void AddEntryTooLargeToCommit(const std::string& conv_uuid,
                                const std::string& entry_uuid) {
    auto conv = mojom::Conversation::New();
    conv->uuid = conv_uuid;
    auto entry = mojom::ConversationTurn::New();
    entry->uuid = entry_uuid;
    entry->character_type = mojom::CharacterType::HUMAN;
    entry->action_type = mojom::ActionType::QUERY;
    entry->text = "Hello";
    entry->selected_text = std::string(kSyncMaxRecordBytes + 1024, 'Z');
    entry->created_time = base::Time::Now();
    ASSERT_TRUE(db_->AddConversation(std::move(conv), {}, std::move(entry)));
    ASSERT_TRUE(db_->UpdateEntityMetadata(syncer::AI_CHAT_CONVERSATION,
                                          "e:" + entry_uuid,
                                          sync_pb::EntityMetadata()));
  }

  base::FilePath db_file_path() {
    return temp_directory_.GetPath().AppendASCII("test_ai_chat.db");
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_directory_;
  std::unique_ptr<AIChatDatabase> db_;
  std::unique_ptr<AIChatSyncBridge> bridge_;
  raw_ptr<syncer::MockDataTypeLocalChangeProcessor> mock_processor_ = nullptr;
  int remote_changes_applied_count_ = 0;
  std::vector<std::string> last_affected_conversation_uuids_;
};

TEST_F(AIChatSyncBridgeTest, SetDatabaseCallsModelReadyToSyncOnce) {
  auto processor = std::make_unique<
      testing::NiceMock<syncer::MockDataTypeLocalChangeProcessor>>();
  auto* processor_ptr = processor.get();
  // Construction alone (before a database is attached) must not report the
  // model ready.
  EXPECT_CALL(*processor_ptr, ModelReadyToSync(_)).Times(0);
  bridge_ = std::make_unique<AIChatSyncBridge>(std::move(processor),
                                               base::DoNothing());
  testing::Mock::VerifyAndClearExpectations(processor_ptr);

  // The first SetDatabase() loads metadata and reports the model ready exactly
  // once; a detach + re-attach (storage toggled off then on) must not report it
  // again, since the change processor keeps its in-memory metadata.
  EXPECT_CALL(*processor_ptr, ModelReadyToSync(_)).Times(1);
  bridge_->SetDatabase(db_.get());
  bridge_->ClearDatabase();
  bridge_->SetDatabase(db_.get());
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

  EXPECT_FALSE(bridge_->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                          syncer::EntityChangeList()));
}

// A merge writes the server's records and doesn't upload them back.
TEST_F(AIChatSyncBridgeTest, MergeFullSyncDataAppliesRemoteRecords) {
  CreateBridge();
  syncer::EntityChangeList changes = MakeConversationAdd("remote-conv");
  changes.push_back(syncer::EntityChange::CreateAdd(
      "e:remote-entry",
      MakeRemoteEntry("remote-conv", "remote-entry", "From remote")));

  EXPECT_CALL(*mock_processor_, Put).Times(0);
  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       std::move(changes))
                   .has_value());

  auto conversations = db_->GetAllConversations();
  ASSERT_EQ(conversations.size(), 1u);
  EXPECT_EQ(conversations[0]->uuid, "remote-conv");
  auto archive = db_->GetConversationData("remote-conv");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  EXPECT_EQ(archive->entries[0]->text, "From remote");
  EXPECT_EQ(remote_changes_applied_count_, 1);
  EXPECT_THAT(last_affected_conversation_uuids_, ElementsAre("remote-conv"));
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
  CreateBridge(/*is_tracking_metadata=*/false);

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

  // The processor lists what to clear, as it does when sync is turned off.
  auto delete_metadata_change_list = bridge_->CreateMetadataChangeList();
  delete_metadata_change_list->ClearMetadata("test-key");
  delete_metadata_change_list->ClearDataTypeState();
  bridge_->ApplyDisableSyncChanges(std::move(delete_metadata_change_list));

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

// With no database attached (storage disabled), the bridge must not touch the
// database or upload anything, and read paths must return empty.
TEST_F(AIChatSyncBridgeTest, NoDatabaseIsInertButStillServesSyncEngine) {
  AddTestConversation("detached-conv", "Detached");
  CreateBridge();
  bridge_->ClearDatabase();

  // No uploads while detached.
  EXPECT_CALL(*mock_processor_, Put(_, _, _)).Times(0);
  syncer::EntityChangeList empty_remote;
  EXPECT_FALSE(bridge_->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                          std::move(empty_remote)));

  // Read paths return empty rather than crashing.
  EXPECT_FALSE(bridge_->GetDataForCommit({"c:detached-conv"})->HasNext());
  EXPECT_FALSE(bridge_->GetAllDataForDebugging()->HasNext());

  // Re-attaching restores normal operation.
  bridge_->SetDatabase(db_.get());
  EXPECT_TRUE(bridge_->GetDataForCommit({"c:detached-conv"})->HasNext());
}

// The bridge must serialize the extracted page text stored alongside the
// conversation (the archive keeps it separate from the associated-content
// metadata), otherwise synced entries would arrive with empty last_contents.
TEST_F(AIChatSyncBridgeTest, GetDataForCommitSerializesAssociatedContentText) {
  const std::string conv_uuid = "conv-with-content";
  const std::string entry_uuid = conv_uuid + "-entry-1";
  const std::string content_uuid = "content-1";

  auto conv = mojom::Conversation::New();
  conv->uuid = conv_uuid;
  conv->title = "With content";
  conv->associated_content.push_back(mojom::AssociatedContent::New(
      content_uuid, mojom::ContentType::PageContent, "page title", 1,
      GURL("https://example.com"), 42, entry_uuid, false));

  auto entry = mojom::ConversationTurn::New();
  entry->uuid = entry_uuid;
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->text = "Hello";
  entry->created_time = base::Time::Now();

  ASSERT_TRUE(db_->AddConversation(std::move(conv), {"the page contents"},
                                   std::move(entry)));
  CreateBridge();

  auto batch = bridge_->GetDataForCommit({"e:" + entry_uuid});
  ASSERT_TRUE(batch->HasNext());
  auto [key, data] = batch->Next();
  const auto& entry_proto = data->specifics.ai_chat_conversation().entry();
  ASSERT_EQ(entry_proto.associated_content_size(), 1);
  ASSERT_TRUE(entry_proto.associated_content(0).has_last_contents());
  EXPECT_EQ(
      ReadCompressibleString(entry_proto.associated_content(0).last_contents()),
      "the page contents");
}

// An entry oversized by a plain, un-omittable field can never be committed.
// Leaving its key out of the batch silently is what Chromium records as
// Sync.DataTypeOrphanMetadata.GetData, so the bridge untracks it explicitly,
// and clears the persisted metadata that untracking alone would leave behind.
TEST_F(AIChatSyncBridgeTest, GetDataForCommitUntracksEntryTooLargeToCommit) {
  CreateBridge();
  ASSERT_NO_FATAL_FAILURE(AddEntryTooLargeToCommit("conv", "entry-1"));

  EXPECT_CALL(*mock_processor_, UntrackEntityForStorageKey("e:entry-1"));
  auto batch = bridge_->GetDataForCommit({"e:entry-1"});
  EXPECT_FALSE(batch->HasNext());

  syncer::MetadataBatch metadata;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&metadata));
  EXPECT_TRUE(metadata.GetAllMetadata().empty());
}

// An entry edited past the budget has nothing to Put, but stays tracked:
// untracking one committed earlier would turn a later local delete into a
// no-op and strand the server's copy.
TEST_F(AIChatSyncBridgeTest, EntryModifiedLeavesTooLargeEntryTracked) {
  CreateBridge();
  ASSERT_NO_FATAL_FAILURE(AddEntryTooLargeToCommit("conv", "entry-1"));

  EXPECT_CALL(*mock_processor_, Put).Times(0);
  EXPECT_CALL(*mock_processor_, UntrackEntityForStorageKey).Times(0);
  bridge_->OnConversationEntryModified("conv", "entry-1");

  syncer::MetadataBatch metadata;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&metadata));
  EXPECT_EQ(metadata.GetAllMetadata().size(), 1u);
}

// The first-sync upload builds entries the same way, so an entry that cannot
// fit is left out while its conversation's metadata record is still sent.
TEST_F(AIChatSyncBridgeTest, MergeFullSyncDataSkipsEntryTooLargeToCommit) {
  ASSERT_NO_FATAL_FAILURE(AddEntryTooLargeToCommit("conv", "entry-1"));
  CreateBridge();

  EXPECT_CALL(*mock_processor_, Put("c:conv", _, _));
  EXPECT_CALL(*mock_processor_, Put("e:entry-1", _, _)).Times(0);
  EXPECT_FALSE(bridge_->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                          syncer::EntityChangeList()));
}

// Each uploaded record carries the time its content last changed, so a device
// receiving it can tell which of two copies is newer.
TEST_F(AIChatSyncBridgeTest, PutSendsModifiedTime) {
  const std::string entry_uuid = AddTestConversation("conv", "Title");
  const base::Time added = base::Time::Now();
  CreateBridge();

  EXPECT_CALL(*mock_processor_, Put("c:conv", HasModifiedTime(added), _));
  EXPECT_CALL(*mock_processor_,
              Put("e:" + entry_uuid, HasModifiedTime(added), _));
  EXPECT_FALSE(bridge_->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                          syncer::EntityChangeList()));
  testing::Mock::VerifyAndClearExpectations(mock_processor_.get());

  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(db_->UpdateConversationTitle("conv", "Renamed"));
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-2";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->text = "Hello again";
  entry->created_time = base::Time::Now();
  ASSERT_TRUE(db_->AddConversationEntry("conv", std::move(entry)));

  EXPECT_CALL(*mock_processor_,
              Put("c:conv", HasModifiedTime(base::Time::Now()), _));
  EXPECT_CALL(*mock_processor_,
              Put("e:entry-2", HasModifiedTime(base::Time::Now()), _));
  bridge_->OnConversationModified("conv");
  bridge_->OnConversationEntryAdded("conv", "entry-2");
}

TEST_F(AIChatSyncBridgeTest, GetDataForCommitSendsModifiedTime) {
  const std::string entry_uuid = AddTestConversation("conv", "Title");
  const base::Time added = base::Time::Now();
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(db_->UpdateConversationTitle("conv", "Renamed"));
  CreateBridge();

  auto batch = bridge_->GetDataForCommit({"c:conv", "e:" + entry_uuid});
  std::map<std::string, base::Time> sent;
  while (batch->HasNext()) {
    auto [key, data] = batch->Next();
    sent[key] =
        GetModifiedTimeFromSpecifics(data->specifics.ai_chat_conversation());
  }
  EXPECT_THAT(sent, ElementsAre(Pair("c:conv", base::Time::Now()),
                                Pair("e:" + entry_uuid, added)));
}

TEST_F(AIChatSyncBridgeTest, ApplyIncrementalSyncChangesDeleteConversation) {
  AddTestConversation("to-delete", "Will Be Deleted");
  CreateBridge();

  ASSERT_FALSE(db_->GetAllConversations().empty());

  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateDelete("c:to-delete", syncer::EntityData()));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  EXPECT_TRUE(db_->GetAllConversations().empty());
}

TEST_F(AIChatSyncBridgeTest, ApplyIncrementalSyncChangesDeleteEntry) {
  const std::string entry_uuid = AddTestConversation("conv-with-entry", "Test");
  CreateBridge();

  // Confirm the entry exists.
  auto archive = db_->GetConversationData("conv-with-entry");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);

  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateDelete("e:" + entry_uuid,
                                                       syncer::EntityData()));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  archive = db_->GetConversationData("conv-with-entry");
  ASSERT_TRUE(archive);
  EXPECT_TRUE(archive->entries.empty());
}

TEST_F(AIChatSyncBridgeTest, ApplyIncrementalSyncChangesUpsertConversation) {
  CreateBridge();

  syncer::EntityData data =
      MakeRemoteConversation("remote-conv", "From remote");
  auto* conv =
      data.specifics.mutable_ai_chat_conversation()->mutable_conversation();
  conv->set_total_tokens(100);
  conv->set_trimmed_tokens(10);
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateAdd("c:remote-conv", std::move(data)));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  auto conversations = db_->GetAllConversations();
  ASSERT_EQ(conversations.size(), 1u);
  EXPECT_EQ(conversations[0]->uuid, "remote-conv");
  EXPECT_EQ(conversations[0]->title, "From remote");
  EXPECT_EQ(conversations[0]->total_tokens, 100u);
  EXPECT_EQ(conversations[0]->trimmed_tokens, 10u);
}

TEST_F(AIChatSyncBridgeTest,
       ApplyIncrementalSyncChangesUpsertEntryCreatesStub) {
  CreateBridge();

  // Send an entry whose parent conversation does not exist locally yet;
  // ApplyRemoteEntry should create a stub conversation row.
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateAdd(
      "e:remote-entry",
      MakeRemoteEntry("remote-conv-stub", "remote-entry", "Hi from remote")));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  auto archive = db_->GetConversationData("remote-conv-stub");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  ASSERT_TRUE(archive->entries[0]->uuid.has_value());
  EXPECT_EQ(*archive->entries[0]->uuid, "remote-entry");
  EXPECT_EQ(archive->entries[0]->text, "Hi from remote");
  // The stub stays hidden until its metadata record arrives.
  EXPECT_TRUE(db_->GetAllConversations().empty());
}

// When an entry arrives before its parent conversation a stub row is
// created. A later metadata record must upgrade the stub in place without
// dropping the entry rows that were attached during the stub phase. Entries
// hold conversation_uuid as a plain column rather than a foreign key, so
// writing the parent row has nothing to cascade into.
TEST_F(AIChatSyncBridgeTest,
       ApplyIncrementalSyncChangesUpgradesStubConversation) {
  CreateBridge();

  // Deliver an entry whose parent does not exist locally.
  {
    syncer::EntityChangeList changes;
    changes.push_back(syncer::EntityChange::CreateAdd(
        "e:orphan-entry", MakeRemoteEntry("conv-to-upgrade", "orphan-entry",
                                          "Pre-metadata entry")));
    ASSERT_FALSE(
        bridge_
            ->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                          std::move(changes))
            .has_value());
  }

  // The stub holds the entry, and stays hidden without its metadata.
  auto stub = db_->GetConversationData("conv-to-upgrade");
  ASSERT_TRUE(stub);
  ASSERT_EQ(stub->entries.size(), 1u);
  EXPECT_TRUE(db_->GetAllConversations().empty());

  // Now deliver the conversation metadata. Stub should be upgraded with real
  // title/tokens, entry must still be present.
  {
    syncer::EntityData data =
        MakeRemoteConversation("conv-to-upgrade", "Real Title");
    auto* conv =
        data.specifics.mutable_ai_chat_conversation()->mutable_conversation();
    conv->set_total_tokens(123);
    conv->set_trimmed_tokens(45);
    syncer::EntityChangeList changes;
    changes.push_back(
        syncer::EntityChange::CreateAdd("c:conv-to-upgrade", std::move(data)));
    ASSERT_FALSE(
        bridge_
            ->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                          std::move(changes))
            .has_value());
  }

  // Title is populated and the entry survived.
  auto conversations = db_->GetAllConversations();
  ASSERT_EQ(conversations.size(), 1u);
  EXPECT_EQ(conversations[0]->uuid, "conv-to-upgrade");
  EXPECT_EQ(conversations[0]->title, "Real Title");
  EXPECT_EQ(conversations[0]->total_tokens, 123u);
  EXPECT_EQ(conversations[0]->trimmed_tokens, 45u);

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
  changes.push_back(syncer::EntityChange::CreateDelete("e:never-existed",
                                                       syncer::EntityData()));

  auto error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(), std::move(changes));
  EXPECT_FALSE(error.has_value());

  // The unrelated existing entry and conversation must be untouched.
  auto after = db_->GetConversationData("conv-with-known-entry");
  ASSERT_TRUE(after);
  EXPECT_EQ(after->entries.size(), 1u);
}

// An applied record keeps the time it carries, which is when the sending
// device last changed it.
TEST_F(AIChatSyncBridgeTest, ApplyIncrementalSyncChangesStoresRecordTime) {
  CreateBridge();
  const base::Time conversation_time = base::Time::Now() - base::Hours(2);
  const base::Time entry_time = base::Time::Now() - base::Hours(1);

  syncer::EntityData conversation = MakeRemoteConversation("conv", "Title");
  conversation.specifics.mutable_ai_chat_conversation()
      ->mutable_conversation()
      ->set_modified_time_windows_epoch_micros(
          conversation_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateAdd("c:conv", std::move(conversation)));
  syncer::EntityData entry = MakeRemoteEntry("conv", "entry", "Hello");
  entry.specifics.mutable_ai_chat_conversation()
      ->mutable_entry()
      ->set_modified_time_windows_epoch_micros(
          entry_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
  changes.push_back(
      syncer::EntityChange::CreateAdd("e:entry", std::move(entry)));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  EXPECT_EQ(db_->GetConversationModifiedTime("conv"), conversation_time);
  EXPECT_EQ(db_->GetEntryModifiedTime("entry"), entry_time);
}

// Whenever a Merge/Apply batch actually changes the local DB, the bridge
// must invoke the on-remote-changes-applied closure exactly once so the
// service can refresh in-memory state and active conversation handlers.
TEST_F(AIChatSyncBridgeTest, NotifiesRemoteChangesAppliedOnAddBatch) {
  CreateBridge();
  ASSERT_EQ(remote_changes_applied_count_, 0);

  ASSERT_FALSE(
      bridge_
          ->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                        MakeConversationAdd("remote-conv"))
          .has_value());

  EXPECT_EQ(remote_changes_applied_count_, 1);
}

TEST_F(AIChatSyncBridgeTest, DoesNotNotifyWhenNoRemoteChangesApplied) {
  CreateBridge();

  ASSERT_FALSE(
      bridge_
          ->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                        syncer::EntityChangeList())
          .has_value());

  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// The notification names only the conversations the batch wrote. A listener
// that refreshed anything else would discard live state that conversation has
// not persisted yet.
TEST_F(AIChatSyncBridgeTest, NotifiesOnlyTheConversationsTheBatchChanged) {
  CreateBridge();
  AddTestConversation("untouched-conv", "Untouched");

  syncer::EntityChangeList changes = MakeConversationAdd("changed-conv");
  changes.push_back(syncer::EntityChange::CreateAdd(
      "e:entry-in-other-conv",
      MakeRemoteEntry("other-conv", "entry-in-other-conv", "Hello")));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  EXPECT_EQ(remote_changes_applied_count_, 1);
  EXPECT_THAT(last_affected_conversation_uuids_,
              UnorderedElementsAre("changed-conv", "other-conv"));
}

// An entry tombstone only carries the entry uuid, so the bridge has to resolve
// the parent conversation before deleting the row, while it still exists.
TEST_F(AIChatSyncBridgeTest, EntryTombstoneNamesTheParentConversation) {
  CreateBridge();
  const std::string entry_uuid = AddTestConversation("conv", "Title");

  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateDelete("e:" + entry_uuid,
                                                       syncer::EntityData()));
  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  EXPECT_EQ(remote_changes_applied_count_, 1);
  EXPECT_THAT(last_affected_conversation_uuids_, ElementsAre("conv"));
}

TEST_F(AIChatSyncBridgeTest, MergeFullSyncDataReturnsErrorWhenApplyFails) {
  CreateBridgeWithFailingDatabase(0);

  auto error = bridge_->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                          MakeConversationAdd("remote-conv"));

  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);
}

TEST_F(AIChatSyncBridgeTest,
       ApplyIncrementalSyncChangesReturnsErrorWhenApplyFails) {
  CreateBridgeWithFailingDatabase(0);

  auto error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(), MakeConversationAdd("remote-conv"));

  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);
  // Nothing applied, so live conversations must not be told to reload.
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

TEST_F(AIChatSyncBridgeTest,
       ApplyIncrementalSyncChangesReturnsErrorWhenDeleteFails) {
  CreateBridgeWithFailingDatabase(0);

  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateDelete("c:to-delete", syncer::EntityData()));

  auto error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(), std::move(changes));

  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);
}

TEST_F(AIChatSyncBridgeTest, NotifiesRemoteChangesAppliedBeforeReturningError) {
  // The first write lands, the second fails partway through the batch.
  CreateBridgeWithFailingDatabase(1);

  syncer::EntityChangeList changes = MakeConversationAdd("first-conv");
  syncer::EntityChangeList second = MakeConversationAdd("second-conv");
  changes.push_back(std::move(second.front()));

  auto error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(), std::move(changes));

  ASSERT_TRUE(error.has_value());
  // The first conversation is committed, so live conversations still need to
  // reload even though the data type is about to stop.
  EXPECT_EQ(remote_changes_applied_count_, 1);
  EXPECT_THAT(last_affected_conversation_uuids_, ElementsAre("first-conv"));
  auto conversations = db_->GetAllConversations();
  ASSERT_EQ(conversations.size(), 1u);
  EXPECT_EQ(conversations[0]->uuid, "first-conv");
}

TEST_F(AIChatSyncBridgeTest,
       ApplyIncrementalSyncChangesIgnoresTombstoneWithEmptyUuid) {
  CreateBridge();

  // Tombstones skip IsEntityDataValid(), so a storage key that is nothing but
  // the prefix must be dropped rather than reaching the database.
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateDelete("c:", syncer::EntityData()));
  changes.push_back(
      syncer::EntityChange::CreateDelete("e:", syncer::EntityData()));

  auto error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(), std::move(changes));

  EXPECT_FALSE(error.has_value());
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// The processor records a batch's progress marker and each record's new
// version before it calls the bridge. If those were saved while the batch
// failed, sync would count the batch as received and never download it again.
TEST_F(AIChatSyncBridgeTest,
       ApplyIncrementalSyncChangesFailureLeavesSyncMetadataUnsaved) {
  CreateBridgeWithFailingDatabase(0);
  ASSERT_NO_FATAL_FAILURE(SaveSyncMetadata("old-token", "c:remote-conv", 1));

  auto error = bridge_->ApplyIncrementalSyncChanges(
      MakeBatchMetadataChanges("new-token", "c:remote-conv", 2),
      MakeConversationAdd("remote-conv"));
  ASSERT_TRUE(error.has_value());

  syncer::MetadataBatch saved;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&saved));
  EXPECT_EQ(saved.GetDataTypeState().progress_marker().token(), "old-token");
  EXPECT_THAT(saved.GetAllMetadata(),
              ElementsAre(Pair("c:remote-conv", HasServerVersion(1))));
}

TEST_F(AIChatSyncBridgeTest,
       MergeFullSyncDataFailureLeavesSyncMetadataUnsaved) {
  CreateBridgeWithFailingDatabase(0);

  auto error = bridge_->MergeFullSyncData(
      MakeBatchMetadataChanges("token", "c:remote-conv", 1),
      MakeConversationAdd("remote-conv"));
  ASSERT_TRUE(error.has_value());

  syncer::MetadataBatch saved;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&saved));
  EXPECT_FALSE(saved.GetDataTypeState().has_progress_marker());
  EXPECT_THAT(saved.GetAllMetadata(), IsEmpty());
}

TEST_F(AIChatSyncBridgeTest, ApplyIncrementalSyncChangesSavesSyncMetadata) {
  CreateBridge();
  ASSERT_NO_FATAL_FAILURE(SaveSyncMetadata("old-token", "c:remote-conv", 1));

  ASSERT_FALSE(
      bridge_
          ->ApplyIncrementalSyncChanges(
              MakeBatchMetadataChanges("new-token", "c:remote-conv", 2),
              MakeConversationAdd("remote-conv"))
          .has_value());

  syncer::MetadataBatch saved;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&saved));
  EXPECT_EQ(saved.GetDataTypeState().progress_marker().token(), "new-token");
  EXPECT_THAT(saved.GetAllMetadata(),
              ElementsAre(Pair("c:remote-conv", HasServerVersion(2))));
}

TEST_F(AIChatSyncBridgeTest, MergeFullSyncDataSavesSyncMetadata) {
  CreateBridge();

  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(
                       MakeBatchMetadataChanges("token", "c:remote-conv", 1),
                       MakeConversationAdd("remote-conv"))
                   .has_value());

  syncer::MetadataBatch saved;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&saved));
  EXPECT_EQ(saved.GetDataTypeState().progress_marker().token(), "token");
  EXPECT_THAT(saved.GetAllMetadata(),
              ElementsAre(Pair("c:remote-conv", HasServerVersion(1))));
}

// After every commit the processor calls ApplyIncrementalSyncChanges() with
// the committed records' new versions and no record changes.
TEST_F(AIChatSyncBridgeTest, MetadataOnlyApplySavesSyncMetadata) {
  CreateBridge();

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       MakeBatchMetadataChanges("token", "c:local-conv", 1),
                       syncer::EntityChangeList())
                   .has_value());

  syncer::MetadataBatch saved;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&saved));
  EXPECT_EQ(saved.GetDataTypeState().progress_marker().token(), "token");
  EXPECT_THAT(saved.GetAllMetadata(),
              ElementsAre(Pair("c:local-conv", HasServerVersion(1))));
}

// A local change has no batch to wait for, so the metadata the processor
// records for it is saved straight away.
TEST_F(AIChatSyncBridgeTest, LocalChangeSavesSyncMetadata) {
  const std::string entry_uuid = AddTestConversation("conv", "Title");
  CreateBridge();
  EXPECT_CALL(*mock_processor_, Put("e:" + entry_uuid, _, _))
      .WillOnce(RecordLocalChangeMetadata());

  bridge_->OnConversationEntryAdded("conv", entry_uuid);

  syncer::MetadataBatch saved;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&saved));
  EXPECT_THAT(saved.GetAllMetadata(),
              ElementsAre(Pair("e:" + entry_uuid, HasServerVersion(1))));
}

// AddOrUpdateAssociatedContent() would insert a repeated associated content
// uuid twice and fail on the primary key, every time the record is applied. So
// the record is skipped, and the rest of the batch still applies.
TEST_F(AIChatSyncBridgeTest, ApplySkipsEntryWithRepeatedAssociatedContentUuid) {
  CreateBridge();

  syncer::EntityData bad_entry = MakeRemoteEntry("conv", "bad-entry", "Hello");
  AddRemoteAssociatedContent(bad_entry, "content");
  AddRemoteAssociatedContent(bad_entry, "content");
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateAdd("e:bad-entry", std::move(bad_entry)));
  changes.push_back(syncer::EntityChange::CreateAdd(
      "e:good-entry", MakeRemoteEntry("conv", "good-entry", "Hello")));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  auto archive = db_->GetConversationData("conv");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  EXPECT_EQ(archive->entries[0]->uuid, "good-entry");
  EXPECT_TRUE(archive->associated_content.empty());
}

// An associated content uuid already held by another conversation would
// collide on insert, every time the record is applied. So the record is
// skipped, the other conversation's content is left alone, and the rest of the
// batch still applies.
TEST_F(AIChatSyncBridgeTest,
       ApplySkipsEntryWithAssociatedContentOfAnotherConversation) {
  auto local = mojom::Conversation::New();
  local->uuid = "local-conv";
  local->associated_content.push_back(mojom::AssociatedContent::New(
      "content", mojom::ContentType::PageContent, "Local page", 1,
      GURL("https://example.com/local"), 100, "local-entry", false));
  auto local_entry = mojom::ConversationTurn::New();
  local_entry->uuid = "local-entry";
  local_entry->character_type = mojom::CharacterType::HUMAN;
  local_entry->action_type = mojom::ActionType::QUERY;
  local_entry->text = "Hello";
  local_entry->created_time = base::Time::Now();
  ASSERT_TRUE(db_->AddConversation(std::move(local), {"Local page text"},
                                   std::move(local_entry)));
  CreateBridge();

  syncer::EntityData bad_entry =
      MakeRemoteEntry("remote-conv", "bad-entry", "Hello");
  AddRemoteAssociatedContent(bad_entry, "content");
  syncer::EntityData good_entry =
      MakeRemoteEntry("remote-conv", "good-entry", "Hello");
  AddRemoteAssociatedContent(good_entry, "other-content");
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateAdd("e:bad-entry", std::move(bad_entry)));
  changes.push_back(
      syncer::EntityChange::CreateAdd("e:good-entry", std::move(good_entry)));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  auto remote = db_->GetConversationData("remote-conv");
  ASSERT_TRUE(remote);
  ASSERT_EQ(remote->entries.size(), 1u);
  EXPECT_EQ(remote->entries[0]->uuid, "good-entry");
  ASSERT_EQ(remote->associated_content.size(), 1u);
  EXPECT_EQ(remote->associated_content[0]->content_uuid, "other-content");

  auto kept = db_->GetConversationData("local-conv");
  ASSERT_TRUE(kept);
  ASSERT_EQ(kept->associated_content.size(), 1u);
  EXPECT_EQ(kept->associated_content[0]->content_uuid, "content");
  EXPECT_EQ(kept->associated_content[0]->content, "Local page text");
}

// With no database attached there is nowhere to save the processor's metadata
// changes, so they're dropped. This guards the change list's destructor CHECK,
// which crashes on changes that were neither saved nor dropped.
TEST_F(AIChatSyncBridgeTest, NoDatabaseDropsPendingSyncMetadata) {
  CreateBridgeWithoutDatabase();

  EXPECT_FALSE(
      bridge_->MergeFullSyncData(MakeBatchMetadataChanges("token", "c:conv", 1),
                                 MakeConversationAdd("conv")));
  EXPECT_FALSE(bridge_->ApplyIncrementalSyncChanges(
      MakeBatchMetadataChanges("token", "c:conv", 2),
      MakeConversationAdd("conv")));
  auto delete_metadata_change_list = bridge_->CreateMetadataChangeList();
  delete_metadata_change_list->ClearMetadata("c:conv");
  delete_metadata_change_list->ClearDataTypeState();
  bridge_->ApplyDisableSyncChanges(std::move(delete_metadata_change_list));
}

// A record this device can't read is skipped rather than failing the batch.
// Its metadata is still saved, so it isn't downloaded again on every start.
TEST_F(AIChatSyncBridgeTest, SkippedRecordSavesSyncMetadata) {
  CreateBridge();
  syncer::EntityData unreadable =
      MakeRemoteEntry("conv", "unreadable-entry", "Hello");
  unreadable.specifics.mutable_ai_chat_conversation()
      ->mutable_entry()
      ->set_character_type(999);
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateAdd("e:unreadable-entry",
                                                    std::move(unreadable)));

  ASSERT_FALSE(
      bridge_
          ->ApplyIncrementalSyncChanges(
              MakeBatchMetadataChanges("token", "e:unreadable-entry", 1),
              std::move(changes))
          .has_value());

  EXPECT_FALSE(db_->GetConversationUuidForEntry("unreadable-entry"));
  syncer::MetadataBatch saved;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&saved));
  EXPECT_EQ(saved.GetDataTypeState().progress_marker().token(), "token");
  EXPECT_THAT(saved.GetAllMetadata(),
              ElementsAre(Pair("e:unreadable-entry", HasServerVersion(1))));
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// A tool output block this client doesn't know, as from a newer sender, skips
// that record, and the rest of the batch still applies.
TEST_F(AIChatSyncBridgeTest, ApplySkipsEntryWithUnknownToolOutputBlock) {
  CreateBridge();
  syncer::EntityData bad_entry = MakeRemoteEntry("conv", "bad-entry", "Hello");
  auto* tool_use = bad_entry.specifics.mutable_ai_chat_conversation()
                       ->mutable_entry()
                       ->add_events()
                       ->mutable_tool_use();
  tool_use->set_tool_name("tool");
  tool_use->set_id("tool-1");
  tool_use->add_output();
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateAdd("e:bad-entry", std::move(bad_entry)));
  changes.push_back(syncer::EntityChange::CreateAdd(
      "e:good-entry", MakeRemoteEntry("conv", "good-entry", "Hello")));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  auto archive = db_->GetConversationData("conv");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  EXPECT_EQ(archive->entries[0]->uuid, "good-entry");
}

// After a batch fails, the processor still holds the batch's new record
// versions in memory. A local update to a record the batch didn't write would
// save that version, and the next start would then drop the batch's change as
// already received. So those updates are skipped: the failing record and the
// ones after it. A record written before the failure already holds the remote
// change, and a record outside the batch is unaffected, so both are updated.
TEST_F(AIChatSyncBridgeTest,
       LocalUpdateAfterFailedBatchSkipsRecordsNotWritten) {
  CreateBridgeWithFailingDatabase(1);
  const std::string entry_uuid = AddTestConversation("local-conv", "Title");
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-2";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->text = "Hello again";
  entry->created_time = base::Time::Now();
  ASSERT_TRUE(db_->AddConversationEntry("local-conv", std::move(entry)));

  syncer::EntityChangeList changes = MakeConversationAdd("written-conv");
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:local-conv", MakeRemoteConversation("local-conv", "Renamed")));
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "e:" + entry_uuid,
      MakeRemoteEntry("local-conv", entry_uuid, "From remote")));
  ASSERT_TRUE(bridge_
                  ->ApplyIncrementalSyncChanges(
                      bridge_->CreateMetadataChangeList(), std::move(changes))
                  .has_value());

  EXPECT_CALL(*mock_processor_, Put("c:written-conv", _, _));
  EXPECT_CALL(*mock_processor_, Put("c:local-conv", _, _)).Times(0);
  EXPECT_CALL(*mock_processor_, Put("e:" + entry_uuid, _, _)).Times(0);
  EXPECT_CALL(*mock_processor_, Put("e:entry-2", _, _));
  bridge_->OnConversationModified("written-conv");
  bridge_->OnConversationModified("local-conv");
  bridge_->OnConversationEntryModified("local-conv", entry_uuid);
  bridge_->OnConversationEntryAdded("local-conv", "entry-2");
}

// A local delete after a failed batch is still sent and saved, even for a
// record the batch didn't write. Saving the tombstone at the batch's version
// is right, since the delete came after the remote change.
TEST_F(AIChatSyncBridgeTest, LocalDeleteAfterFailedBatchIsSent) {
  CreateBridgeWithFailingDatabase(0);
  const std::string entry_uuid = AddTestConversation("conv", "Title");

  syncer::EntityChangeList changes = MakeConversationAdd("conv");
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "e:" + entry_uuid, MakeRemoteEntry("conv", entry_uuid, "From remote")));
  ASSERT_TRUE(bridge_
                  ->ApplyIncrementalSyncChanges(
                      bridge_->CreateMetadataChangeList(), std::move(changes))
                  .has_value());

  EXPECT_CALL(*mock_processor_, Delete("e:" + entry_uuid, _, _))
      .WillOnce(RecordLocalChangeMetadata());
  EXPECT_CALL(*mock_processor_, Delete("c:conv", _, _))
      .WillOnce(RecordLocalChangeMetadata());
  bridge_->OnConversationDeleted("conv");

  syncer::MetadataBatch saved;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&saved));
  EXPECT_THAT(saved.GetAllMetadata(),
              ElementsAre(Pair("c:conv", _), Pair("e:" + entry_uuid, _)));
}

// After a first-sync merge fails, first sync runs again on the next start and
// applies the server's records over this device's, so no local update is sent
// this session.
TEST_F(AIChatSyncBridgeTest, LocalUpdateAfterFailedMergeIsSkipped) {
  CreateBridgeWithFailingDatabase(0);
  const std::string entry_uuid = AddTestConversation("local-conv", "Title");

  EXPECT_CALL(*mock_processor_, Put).Times(0);
  ASSERT_TRUE(bridge_
                  ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                      MakeConversationAdd("remote-conv"))
                  .has_value());

  bridge_->OnConversationModified("local-conv");
  bridge_->OnConversationEntryModified("local-conv", entry_uuid);
}

}  // namespace
}  // namespace ai_chat
