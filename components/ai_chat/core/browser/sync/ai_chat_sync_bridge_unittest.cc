// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_bridge.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/metadata_change_list.h"
#include "components/sync/model/model_error.h"
#include "components/sync/protocol/entity_data.h"
#include "components/sync/test/mock_data_type_local_change_processor.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {
namespace {

using testing::_;

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

  bool ApplyRemoteConversationMetadata(
      mojom::ConversationPtr conversation) override {
    return ConsumeWrite() && AIChatDatabase::ApplyRemoteConversationMetadata(
                                 std::move(conversation));
  }

  bool ApplyRemoteEntry(
      std::string_view conversation_uuid,
      mojom::ConversationTurnPtr entry,
      std::vector<mojom::AssociatedContentPtr> associated_content,
      std::vector<std::string> contents) override {
    return ConsumeWrite() &&
           AIChatDatabase::ApplyRemoteEntry(conversation_uuid, std::move(entry),
                                            std::move(associated_content),
                                            std::move(contents));
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

// A database whose batch-wide read of entries with edit revisions fails.
class EditRevisionsUnreadableAIChatDatabase : public AIChatDatabase {
 public:
  using AIChatDatabase::AIChatDatabase;

  std::optional<base::flat_set<std::string>> GetEntryUuidsWithEditRevisions()
      override {
    return std::nullopt;
  }
};

// A single remote conversation add.
syncer::EntityChangeList MakeConversationAdd(const std::string& uuid) {
  syncer::EntityChangeList changes;
  syncer::EntityData data;
  data.specifics.mutable_ai_chat_conversation()
      ->mutable_conversation()
      ->set_uuid(uuid);
  changes.push_back(
      syncer::EntityChange::CreateAdd("c:" + uuid, std::move(data)));
  return changes;
}

// An entry record for a single turn, as another device would send it.
syncer::EntityData MakeRemoteEntry(
    const std::string& conv_uuid,
    const std::string& entry_uuid,
    const std::string& text,
    mojom::CharacterType character_type = mojom::CharacterType::HUMAN) {
  syncer::EntityData data;
  auto* entry = data.specifics.mutable_ai_chat_conversation()->mutable_entry();
  entry->set_uuid(entry_uuid);
  entry->set_conversation_uuid(conv_uuid);
  entry->set_entry_text(text);
  entry->set_character_type(std::to_underlying(character_type));
  entry->set_action_type(std::to_underlying(mojom::ActionType::QUERY));
  entry->set_created_time_windows_epoch_micros(
      base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds());
  return data;
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

  // Adds a conversation whose single entry has been edited once locally, the
  // shape sync cannot represent: the head carries the original text and the
  // revision carries what the user now sees.
  void SeedEditedEntry(const std::string& conv_uuid,
                       const std::string& entry_uuid) {
    auto conv = mojom::Conversation::New();
    conv->uuid = conv_uuid;
    conv->title = "Local";
    auto entry = mojom::ConversationTurn::New();
    entry->uuid = entry_uuid;
    entry->character_type = mojom::CharacterType::HUMAN;
    entry->action_type = mojom::ActionType::QUERY;
    entry->text = "Original question";
    entry->created_time = base::Time::Now();
    auto edit = entry->Clone();
    edit->uuid = "edit-1";
    edit->text = "Edited question";
    edit->created_time = base::Time::Now() + base::Minutes(1);
    entry->edits.emplace();
    entry->edits->push_back(std::move(edit));
    ASSERT_TRUE(db_->AddConversation(std::move(conv), {}, std::move(entry)));
  }

  // Delivers an update for |entry_uuid| carrying |text| as the collapsed
  // latest-revision content, as another device that edited the turn would.
  void ApplyRemoteRevision(const std::string& conv_uuid,
                           const std::string& entry_uuid,
                           const std::string& text) {
    syncer::EntityChangeList changes;
    changes.push_back(syncer::EntityChange::CreateUpdate(
        "e:" + entry_uuid, MakeRemoteEntry(conv_uuid, entry_uuid, text)));
    auto error = bridge_->ApplyIncrementalSyncChanges(
        bridge_->CreateMetadataChangeList(), std::move(changes));
    ASSERT_FALSE(error.has_value());
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

  // Adds a conversation whose single assistant entry carries |completion|.
  void SeedCompletion(const std::string& conv_uuid,
                      const std::string& entry_uuid,
                      const std::string& completion) {
    auto conv = mojom::Conversation::New();
    conv->uuid = conv_uuid;
    conv->title = "Local";
    auto entry = mojom::ConversationTurn::New();
    entry->uuid = entry_uuid;
    entry->character_type = mojom::CharacterType::ASSISTANT;
    entry->action_type = mojom::ActionType::QUERY;
    entry->created_time = base::Time::Now();
    entry->events.emplace();
    entry->events->push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(completion)));
    ASSERT_TRUE(db_->AddConversation(std::move(conv), {}, std::move(entry)));
  }

  // Delivers an update for |entry_uuid| whose completion the sender omitted to
  // fit the size budget, leaving only the hash of |sender_completion|.
  void ApplyRemoteOmittedCompletion(const std::string& conv_uuid,
                                    const std::string& entry_uuid,
                                    const std::string& sender_completion) {
    syncer::EntityData data = MakeRemoteEntry(conv_uuid, entry_uuid, "",
                                              mojom::CharacterType::ASSISTANT);
    auto* event = data.specifics.mutable_ai_chat_conversation()
                      ->mutable_entry()
                      ->add_events();
    WriteCompressibleString(sender_completion, event->mutable_completion());
    OmitCompressibleString(event->mutable_completion());
    ASSERT_TRUE(event->completion().has_omitted_content_hash());
    syncer::EntityChangeList changes;
    changes.push_back(
        syncer::EntityChange::CreateAdd("e:" + entry_uuid, std::move(data)));
    ASSERT_FALSE(
        bridge_
            ->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                          std::move(changes))
            .has_value());
  }

  // Expects the single entry of |conv_uuid| to hold one completion event
  // reading |expected|.
  void ExpectCompletion(const std::string& conv_uuid,
                        std::string_view expected) {
    auto archive = db_->GetConversationData(conv_uuid);
    ASSERT_TRUE(archive);
    ASSERT_EQ(archive->entries.size(), 1u);
    const auto& events = archive->entries[0]->events;
    ASSERT_TRUE(events.has_value());
    ASSERT_EQ(events->size(), 1u);
    ASSERT_TRUE(events->front()->is_completion_event());
    EXPECT_EQ(events->front()->get_completion_event()->completion, expected);
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

  syncer::EntityChangeList empty_remote;
  bridge_->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                             std::move(empty_remote));
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

  db_->AddConversation(std::move(conv), {"the page contents"},
                       std::move(entry));
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
  syncer::EntityData delete_data;
  auto* entry =
      delete_data.specifics.mutable_ai_chat_conversation()->mutable_entry();
  entry->set_uuid(entry_uuid);
  entry->set_conversation_uuid("conv-with-entry");
  changes.push_back(syncer::EntityChange::CreateDelete("e:" + entry_uuid,
                                                       std::move(delete_data)));

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

  // Stub exists with no title yet, entry is attached.
  auto stub = db_->GetConversationData("conv-to-upgrade");
  ASSERT_TRUE(stub);
  ASSERT_EQ(stub->entries.size(), 1u);

  // Now deliver the conversation metadata. Stub should be upgraded with real
  // title/tokens, entry must still be present.
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

// When the remote sender omits AC last_contents to fit the size budget, the
// bridge must restore the local text (matched by content hash) rather than
// overwriting it with empty.
TEST_F(AIChatSyncBridgeTest,
       ApplyRemoteEntryWithOmittedLastContentsRestoresLocal) {
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
      std::optional<std::string>("entry-preserve"), false));
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
  syncer::EntityData data =
      MakeRemoteEntry("conv-preserve", "entry-preserve", "Question?");
  auto* remote_ac = data.specifics.mutable_ai_chat_conversation()
                        ->mutable_entry()
                        ->add_associated_content();
  remote_ac->set_uuid("ac-preserve");
  remote_ac->set_title("Local page");
  remote_ac->set_url("https://example.com/local");
  remote_ac->set_content_type(
      std::to_underlying(mojom::ContentType::PageContent));
  remote_ac->set_content_used_percentage(80);
  // The sender omitted the AC text to fit the budget, leaving behind a hash of
  // the original content (identical to what the receiver holds locally).
  WriteCompressibleString("ORIGINAL local page text",
                          remote_ac->mutable_last_contents());
  OmitCompressibleString(remote_ac->mutable_last_contents());

  changes.push_back(
      syncer::EntityChange::CreateAdd("e:entry-preserve", std::move(data)));
  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  // The local AC's content text must be preserved.
  auto archive = db_->GetConversationData("conv-preserve");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->associated_content.size(), 1u);
  EXPECT_EQ(archive->associated_content[0]->content_uuid, "ac-preserve");
  EXPECT_EQ(archive->associated_content[0]->content,
            "ORIGINAL local page text");
}

// A completion lives inside the entry's events array, whose items have no
// stable identity, so it is restored by content hash. When the sender omits it
// to fit the budget, the bridge must restore the byte-identical local text.
TEST_F(AIChatSyncBridgeTest,
       ApplyRemoteEntryWithOmittedCompletionRestoresLocal) {
  const std::string completion_text =
      "The assistant's long and detailed answer that was omitted for sync.";
  ASSERT_NO_FATAL_FAILURE(
      SeedCompletion("conv-completion", "entry-completion", completion_text));
  CreateBridge();

  ASSERT_NO_FATAL_FAILURE(ApplyRemoteOmittedCompletion(
      "conv-completion", "entry-completion", completion_text));

  ExpectCompletion("conv-completion", completion_text);
}

// Restoring requires the local copy to hash the same as what the sender
// omitted, so a diverging local completion is not restored in its place.
TEST_F(AIChatSyncBridgeTest,
       ApplyRemoteEntryWithOmittedCompletionDoesNotRestoreMismatch) {
  ASSERT_NO_FATAL_FAILURE(SeedCompletion("conv-completion", "entry-completion",
                                         "This device's answer."));
  CreateBridge();

  ASSERT_NO_FATAL_FAILURE(ApplyRemoteOmittedCompletion(
      "conv-completion", "entry-completion", "The sender's different answer."));

  ExpectCompletion("conv-completion", "");
}

// Sync collapses an edited turn to its latest revision, so a full-replace on
// apply would destroy edit history this device may be the only holder of. The
// incoming content has to land as one more revision behind the local head.
TEST_F(AIChatSyncBridgeTest, ApplyRemoteEntryKeepsLocalEditHistory) {
  ASSERT_NO_FATAL_FAILURE(SeedEditedEntry("conv-edits", "entry-edits"));
  CreateBridge();

  ASSERT_NO_FATAL_FAILURE(
      ApplyRemoteRevision("conv-edits", "entry-edits", "Revised elsewhere"));

  auto archive = db_->GetConversationData("conv-edits");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  const auto& applied = archive->entries[0];
  // The head still holds the original text, not the collapsed latest revision.
  EXPECT_EQ(applied->text, "Original question");
  ASSERT_TRUE(applied->edits.has_value());
  ASSERT_EQ(applied->edits->size(), 2u);
  EXPECT_EQ((*applied->edits)[0]->uuid, "edit-1");
  EXPECT_EQ((*applied->edits)[0]->text, "Edited question");
  EXPECT_EQ((*applied->edits)[1]->text, "Revised elsewhere");
}

// Unchecking "AI Chat" in sync settings stops the type with CLEAR_METADATA,
// which clears metadata but not data, so re-checking it replays every record
// through MergeFullSyncData against a populated database. Each replayed record
// is this device's own collapsed content, so nothing may be appended.
TEST_F(AIChatSyncBridgeTest, MergeFullSyncDataDoesNotDuplicateLocalEdits) {
  ASSERT_NO_FATAL_FAILURE(SeedEditedEntry("conv-edits", "entry-edits"));
  CreateBridge();

  auto local = db_->GetConversationEntryWithEdits("entry-edits");
  ASSERT_TRUE(local);
  syncer::EntityChangeList changes;
  syncer::EntityData data;
  *data.specifics.mutable_ai_chat_conversation() =
      EntryToSpecifics("conv-edits", *local, {});
  changes.push_back(
      syncer::EntityChange::CreateAdd("e:entry-edits", std::move(data)));
  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       std::move(changes))
                   .has_value());

  auto archive = db_->GetConversationData("conv-edits");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  const auto& applied = archive->entries[0];
  EXPECT_EQ(applied->text, "Original question");
  ASSERT_TRUE(applied->edits.has_value());
  ASSERT_EQ(applied->edits->size(), 1u);
  EXPECT_EQ((*applied->edits)[0]->uuid, "edit-1");
  EXPECT_EQ((*applied->edits)[0]->text, "Edited question");
}

// The sender hashes the file bytes of the revision it serialized, which for an
// edited turn is the latest edit, so the restore must look there rather than at
// the head's files.
TEST_F(AIChatSyncBridgeTest, MergeFullSyncDataRestoresOmittedFileOfLatestEdit) {
  auto conv = mojom::Conversation::New();
  conv->uuid = "conv-files";
  conv->title = "Local";
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-files";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->text = "Original question";
  entry->created_time = base::Time::Now();
  entry->uploaded_files.emplace();
  entry->uploaded_files->push_back(
      mojom::UploadedFile::New("head.png", 3, std::vector<uint8_t>{1, 2, 3},
                               mojom::UploadedFileType::kImage, std::nullopt));
  auto edit = entry->Clone();
  edit->uuid = "edit-1";
  edit->text = "Edited question";
  edit->created_time = base::Time::Now() + base::Minutes(1);
  edit->uploaded_files->front() =
      mojom::UploadedFile::New("edit.png", 3, std::vector<uint8_t>{4, 5, 6},
                               mojom::UploadedFileType::kImage, std::nullopt);
  entry->edits.emplace();
  entry->edits->push_back(std::move(edit));
  ASSERT_TRUE(db_->AddConversation(std::move(conv), {}, std::move(entry)));
  CreateBridge();

  auto local = db_->GetConversationEntryWithEdits("entry-files");
  ASSERT_TRUE(local);
  syncer::EntityData data;
  auto* specifics = data.specifics.mutable_ai_chat_conversation();
  *specifics = EntryToSpecifics("conv-files", *local, {});
  ASSERT_EQ(specifics->entry().uploaded_files_size(), 1);
  OmitUploadedFileData(specifics->mutable_entry()->mutable_uploaded_files(0));
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateAdd("e:entry-files", std::move(data)));
  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       std::move(changes))
                   .has_value());

  auto applied = db_->GetConversationEntryWithEdits("entry-files");
  ASSERT_TRUE(applied);
  ASSERT_TRUE(applied->edits.has_value());
  ASSERT_EQ(applied->edits->size(), 1u);
  const auto& files = applied->edits->front()->uploaded_files;
  ASSERT_TRUE(files.has_value());
  ASSERT_EQ(files->size(), 1u);
  EXPECT_EQ(files->front()->data, std::vector<uint8_t>({4, 5, 6}));
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
              testing::UnorderedElementsAre("changed-conv", "other-conv"));
}

// An entry tombstone only carries the entry uuid, so the bridge has to resolve
// the parent conversation before deleting the row — afterwards it is gone.
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
  EXPECT_THAT(last_affected_conversation_uuids_, testing::ElementsAre("conv"));
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

// Without the snapshot, an entry's local edit revisions would look absent and
// the full-replace apply would delete them, so the batch fails instead.
TEST_F(AIChatSyncBridgeTest, ApplyFailsWhenEditRevisionsCannotBeRead) {
  db_.reset();
  db_ = std::make_unique<EditRevisionsUnreadableAIChatDatabase>(
      db_file_path(), os_crypt_async::GetTestEncryptorForTesting());
  CreateBridge();

  auto merge_error = bridge_->MergeFullSyncData(
      bridge_->CreateMetadataChangeList(), MakeConversationAdd("merged-conv"));
  ASSERT_TRUE(merge_error.has_value());
  EXPECT_EQ(merge_error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);

  auto apply_error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(), MakeConversationAdd("applied-conv"));
  ASSERT_TRUE(apply_error.has_value());
  EXPECT_EQ(apply_error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);

  EXPECT_TRUE(db_->GetAllConversations().empty());
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
  EXPECT_THAT(last_affected_conversation_uuids_,
              testing::ElementsAre("first-conv"));
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

}  // namespace
}  // namespace ai_chat
