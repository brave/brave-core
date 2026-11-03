// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_bridge.h"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/sync/model/conflict_resolution.h"
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

// Matches the result of AIChatDatabase::GetConversationModifiedTime() or
// GetEntryModifiedTime() for a row whose modified time is |time|.
MATCHER_P(IsModifiedAt, time, "") {
  return arg && arg->exists && arg->time == time;
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

// A database whose modified time reads fail.
class UnreadableTimesAIChatDatabase : public AIChatDatabase {
 public:
  using AIChatDatabase::AIChatDatabase;

  std::optional<ModifiedTime> GetConversationModifiedTime(
      std::string_view conversation_uuid) override {
    return std::nullopt;
  }

  std::optional<ModifiedTime> GetEntryModifiedTime(
      std::string_view conversation_entry_uuid) override {
    return std::nullopt;
  }
};

// A database whose read of the entries that have edit revisions fails, and
// which counts those reads.
class UnreadableEditRevisionsAIChatDatabase : public AIChatDatabase {
 public:
  using AIChatDatabase::AIChatDatabase;

  std::optional<base::flat_set<std::string>> GetEditedEntryUuids() override {
    ++read_count_;
    return std::nullopt;
  }

  int read_count() const { return read_count_; }

 private:
  int read_count_ = 0;
};

// A database whose reads of an entry, with or without its edit revisions, and
// of its associated content texts fail.
class UnreadableEntryAIChatDatabase : public AIChatDatabase {
 public:
  using AIChatDatabase::AIChatDatabase;

  std::optional<mojom::ConversationTurnPtr> GetConversationEntryWithEdits(
      std::string_view conversation_entry_uuid) override {
    return std::nullopt;
  }

  std::optional<mojom::ConversationTurnPtr> GetConversationEntryWithoutEdits(
      std::string_view conversation_entry_uuid) override {
    return std::nullopt;
  }

  std::optional<std::vector<std::string>> GetAssociatedContentTexts(
      std::string_view conversation_entry_uuid) override {
    return std::nullopt;
  }
};

// A database whose read of an entry's associated content texts fails.
class UnreadablePageTextsAIChatDatabase : public AIChatDatabase {
 public:
  using AIChatDatabase::AIChatDatabase;

  std::optional<std::vector<std::string>> GetAssociatedContentTexts(
      std::string_view conversation_entry_uuid) override {
    return std::nullopt;
  }
};

// Returns |data|, a conversation metadata or entry record, carrying |time| as
// its modified time.
syncer::EntityData WithModifiedTime(syncer::EntityData data, base::Time time) {
  auto* specifics = data.specifics.mutable_ai_chat_conversation();
  const int64_t micros = time.ToDeltaSinceWindowsEpoch().InMicroseconds();
  if (specifics->has_conversation()) {
    specifics->mutable_conversation()->set_modified_time_windows_epoch_micros(
        micros);
  } else {
    specifics->mutable_entry()->set_modified_time_windows_epoch_micros(micros);
  }
  return data;
}

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
    db_ = std::make_unique<AIChatDatabase>(db_file_path(), encryptor_);
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
    db_ = std::make_unique<FailingAIChatDatabase>(db_file_path(), encryptor_,
                                                  writes_before_failure);
    CreateBridge();
  }

  // Replaces the database with one whose modified time reads fail, then
  // creates the bridge over it.
  void CreateBridgeWithUnreadableTimes() {
    db_.reset();
    db_ = std::make_unique<UnreadableTimesAIChatDatabase>(db_file_path(),
                                                          encryptor_);
    CreateBridge();
  }

  // Stands in for a browser restart: a new bridge and processor, over the same
  // database file opened again, now without failures.
  void RestartWithWorkingDatabase() {
    mock_processor_ = nullptr;
    bridge_.reset();
    db_.reset();
    db_ = std::make_unique<AIChatDatabase>(db_file_path(), encryptor_);
    CreateBridge();
  }

  // Returns this device's records for |storage_keys| as remote changes, as the
  // server sends them back once this device has uploaded them.
  syncer::EntityChangeList GetLocalRecordsAsRemoteChanges(
      std::vector<std::string> storage_keys) {
    syncer::EntityChangeList changes;
    auto batch = bridge_->GetDataForCommit(std::move(storage_keys));
    while (batch->HasNext()) {
      auto [key, data] = batch->Next();
      changes.push_back(syncer::EntityChange::CreateAdd(key, std::move(*data)));
    }
    return changes;
  }

  // Runs the tasks already posted to this sequence, such as the one that sends
  // the local copies ApplyIncrementalSyncChanges() kept.
  void RunPostedTasks() {
    base::RunLoop run_loop;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, run_loop.QuitClosure());
    run_loop.Run();
  }

  // Returns the title of the listed conversation |uuid|, or nullopt when it
  // isn't listed.
  std::optional<std::string> GetConversationTitle(const std::string& uuid) {
    for (const auto& conversation : db_->GetAllConversations()) {
      if (conversation->uuid == uuid) {
        return conversation->title;
      }
    }
    return std::nullopt;
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

  // Adds a conversation whose single entry was edited once on this device, as
  // the edit is saved: the entry, asked a minute ago, holds the original text,
  // and its revision "edit-1", made now, holds what the user sees. Sync sends
  // only the revision's content, under the entry's uuid. |original_files| and
  // |revision_files|, when given, are the files uploaded with each.
  void SeedEditedEntry(
      const std::string& conv_uuid,
      const std::string& entry_uuid,
      std::vector<mojom::UploadedFilePtr> original_files = {},
      std::vector<mojom::UploadedFilePtr> revision_files = {}) {
    auto conv = mojom::Conversation::New();
    conv->uuid = conv_uuid;
    conv->title = "Local";
    auto entry = mojom::ConversationTurn::New();
    entry->uuid = entry_uuid;
    entry->character_type = mojom::CharacterType::HUMAN;
    entry->action_type = mojom::ActionType::QUERY;
    entry->text = "Original question";
    entry->created_time = base::Time::Now() - base::Minutes(1);
    auto edit = entry->Clone();
    edit->uuid = "edit-1";
    edit->text = "Edited question";
    edit->created_time = base::Time::Now();
    if (!original_files.empty()) {
      entry->uploaded_files = std::move(original_files);
    }
    if (!revision_files.empty()) {
      edit->uploaded_files = std::move(revision_files);
    }
    entry->edits.emplace();
    entry->edits->push_back(std::move(edit));
    ASSERT_TRUE(db_->AddConversation(std::move(conv), {}, std::move(entry)));
  }

  // Adds conversation "conv" whose single answer "entry" was edited once on
  // this device: the entry holds the original completion, and its revision
  // "edit-1" holds |revision_events|. |associated_content| and |contents| are
  // the conversation's.
  void SeedEditedAnswer(
      std::vector<mojom::ConversationEntryEventPtr> revision_events,
      std::vector<mojom::AssociatedContentPtr> associated_content,
      std::vector<std::string> contents) {
    auto conv = mojom::Conversation::New();
    conv->uuid = "conv";
    conv->title = "Local";
    conv->associated_content = std::move(associated_content);
    auto entry = mojom::ConversationTurn::New();
    entry->uuid = "entry";
    entry->character_type = mojom::CharacterType::ASSISTANT;
    entry->action_type = mojom::ActionType::RESPONSE;
    entry->created_time = base::Time::Now() - base::Minutes(1);
    entry->events.emplace();
    entry->events->push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New("Original answer")));
    auto edit = mojom::ConversationTurn::New();
    edit->uuid = "edit-1";
    edit->character_type = mojom::CharacterType::ASSISTANT;
    edit->action_type = mojom::ActionType::RESPONSE;
    edit->created_time = base::Time::Now();
    edit->events = std::move(revision_events);
    entry->edits.emplace();
    entry->edits->push_back(std::move(edit));
    ASSERT_TRUE(db_->AddConversation(std::move(conv), std::move(contents),
                                     std::move(entry)));
  }

  // Returns the texts of the single entry of |conv_uuid|: the entry's own,
  // then each of its revisions' in order.
  std::vector<std::string> GetEntryAndRevisionTexts(
      const std::string& conv_uuid) {
    auto archive = db_->GetConversationData(conv_uuid);
    if (!archive || archive->entries.size() != 1u) {
      return {};
    }
    const auto& entry = archive->entries[0];
    std::vector<std::string> texts = {entry->text};
    if (entry->edits) {
      for (const auto& edit : *entry->edits) {
        texts.push_back(edit->text);
      }
    }
    return texts;
  }

  // Adds conversation "conv" whose single answer "entry" holds |completion|.
  void SeedCompletion(const std::string& completion) {
    auto conv = mojom::Conversation::New();
    conv->uuid = "conv";
    conv->title = "Local";
    auto entry = mojom::ConversationTurn::New();
    entry->uuid = "entry";
    entry->character_type = mojom::CharacterType::ASSISTANT;
    entry->action_type = mojom::ActionType::RESPONSE;
    entry->created_time = base::Time::Now();
    entry->events.emplace();
    entry->events->push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(completion)));
    ASSERT_TRUE(db_->AddConversation(std::move(conv), {}, std::move(entry)));
  }

  // Returns an update to "entry" from another device, dated |modified_time|,
  // whose completion the sender omitted to fit the size limit, sending the
  // hash of |sender_completion| instead.
  syncer::EntityChangeList MakeOmittedCompletionUpdate(
      const std::string& sender_completion,
      base::Time modified_time) {
    syncer::EntityData data =
        WithModifiedTime(MakeRemoteEntry("conv", "entry", ""), modified_time);
    auto* remote =
        data.specifics.mutable_ai_chat_conversation()->mutable_entry();
    remote->set_character_type(
        std::to_underlying(mojom::CharacterType::ASSISTANT));
    remote->set_action_type(std::to_underlying(mojom::ActionType::RESPONSE));
    auto* completion = remote->add_events()->mutable_completion();
    WriteCompressibleString(sender_completion, completion);
    OmitCompressibleString(completion);
    syncer::EntityChangeList changes;
    changes.push_back(
        syncer::EntityChange::CreateUpdate("e:entry", std::move(data)));
    return changes;
  }

  // Returns the completions of the single entry of |conv_uuid|.
  std::vector<std::string> GetCompletions(const std::string& conv_uuid) {
    auto archive = db_->GetConversationData(conv_uuid);
    if (!archive || archive->entries.size() != 1u ||
        !archive->entries[0]->events) {
      return {};
    }
    std::vector<std::string> completions;
    for (const auto& event : *archive->entries[0]->events) {
      if (event->is_completion_event()) {
        completions.push_back(event->get_completion_event()->completion);
      }
    }
    return completions;
  }

  base::FilePath db_file_path() {
    return temp_directory_.GetPath().AppendASCII("test_ai_chat.db");
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_directory_;
  // Shared by every database the test opens, so each can read what another
  // wrote.
  scoped_refptr<os_crypt_async::Encryptor> encryptor_ =
      os_crypt_async::GetTestEncryptorForTesting();
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

  EXPECT_THAT(db_->GetConversationModifiedTime("conv"),
              IsModifiedAt(conversation_time));
  EXPECT_THAT(db_->GetEntryModifiedTime("entry"), IsModifiedAt(entry_time));
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

  // Changed on another device after this device's copies.
  task_environment_.FastForwardBy(base::Minutes(1));
  syncer::EntityChangeList changes = MakeConversationAdd("written-conv");
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:local-conv",
      WithModifiedTime(MakeRemoteConversation("local-conv", "Renamed"),
                       base::Time::Now())));
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "e:" + entry_uuid,
      WithModifiedTime(MakeRemoteEntry("local-conv", entry_uuid, "From remote"),
                       base::Time::Now())));
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

// A local update skipped after a failed batch was made after the batch's
// change, so it's newer. The next start downloads the batch again, keeps the
// local update and sends it.
TEST_F(AIChatSyncBridgeTest, SkippedLocalUpdateIsSentAfterRestart) {
  CreateBridgeWithFailingDatabase(0);
  AddTestConversation("conv", "Title");
  task_environment_.FastForwardBy(base::Minutes(1));
  const base::Time remote_time = base::Time::Now();
  auto make_batch = [&] {
    syncer::EntityChangeList changes;
    changes.push_back(syncer::EntityChange::CreateUpdate(
        "c:conv",
        WithModifiedTime(MakeRemoteConversation("conv", "Renamed remotely"),
                         remote_time)));
    return changes;
  };
  ASSERT_TRUE(bridge_
                  ->ApplyIncrementalSyncChanges(
                      bridge_->CreateMetadataChangeList(), make_batch())
                  .has_value());
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(db_->UpdateConversationTitle("conv", "Renamed locally"));
  EXPECT_CALL(*mock_processor_, Put).Times(0);
  bridge_->OnConversationModified("conv");

  RestartWithWorkingDatabase();
  EXPECT_CALL(*mock_processor_,
              Put("c:conv", HasModifiedTime(base::Time::Now()), _));
  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), make_batch())
                   .has_value());
  RunPostedTasks();
  EXPECT_EQ(GetConversationTitle("conv"), "Renamed locally");
}

// A local delete after a failed batch is still sent and saved, even for a
// record the batch didn't write. Saving the tombstone at the batch's version
// is right, since the delete came after the remote change.
TEST_F(AIChatSyncBridgeTest, LocalDeleteAfterFailedBatchIsSent) {
  CreateBridgeWithFailingDatabase(0);
  const std::string entry_uuid = AddTestConversation("conv", "Title");

  // Changed on another device after this device's copies.
  task_environment_.FastForwardBy(base::Minutes(1));
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:conv", WithModifiedTime(MakeRemoteConversation("conv", "Remote"),
                                 base::Time::Now())));
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "e:" + entry_uuid,
      WithModifiedTime(MakeRemoteEntry("conv", entry_uuid, "From remote"),
                       base::Time::Now())));
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

// After a first-sync merge fails, first sync runs again on the next start, so
// no local update is sent this session.
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

// A local update skipped after a failed first-sync merge is newer than the
// server's copy, so the next start's merge keeps it and sends it.
TEST_F(AIChatSyncBridgeTest, SkippedLocalUpdateIsSentByTheNextMerge) {
  CreateBridgeWithFailingDatabase(0);
  const std::string entry_uuid = AddTestConversation("local-conv", "Title");
  const base::Time added = base::Time::Now();
  // The server holds this device's conversation as first uploaded, and one
  // from another device.
  auto make_server_records = [&] {
    syncer::EntityChangeList changes = MakeConversationAdd("remote-conv");
    changes.push_back(syncer::EntityChange::CreateAdd(
        "c:local-conv",
        WithModifiedTime(MakeRemoteConversation("local-conv", "Title"),
                         added)));
    return changes;
  };
  ASSERT_TRUE(bridge_
                  ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                      make_server_records())
                  .has_value());
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(db_->UpdateConversationTitle("local-conv", "Renamed"));
  EXPECT_CALL(*mock_processor_, Put).Times(0);
  bridge_->OnConversationModified("local-conv");

  RestartWithWorkingDatabase();
  EXPECT_CALL(*mock_processor_,
              Put("c:local-conv", HasModifiedTime(base::Time::Now()), _));
  EXPECT_CALL(*mock_processor_, Put("e:" + entry_uuid, _, _));
  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       make_server_records())
                   .has_value());
  EXPECT_EQ(GetConversationTitle("local-conv"), "Renamed");
  EXPECT_EQ(GetConversationTitle("remote-conv"), "Remote");
}

// A conversation renamed while AI Chat sync was off wasn't sent, so the
// server's copy is older. A first-sync merge keeps the rename and sends it,
// with its sync metadata saved along with the batch's. The entry the server
// already holds at the same time is left alone and not sent again.
TEST_F(AIChatSyncBridgeTest, MergeKeepsRenameTheServerLacks) {
  const std::string entry_uuid = AddTestConversation("conv", "Title");
  CreateBridge();
  syncer::EntityChangeList server_records =
      GetLocalRecordsAsRemoteChanges({"c:conv", "e:" + entry_uuid});
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(db_->UpdateConversationTitle("conv", "Renamed"));

  EXPECT_CALL(*mock_processor_,
              Put("c:conv", HasModifiedTime(base::Time::Now()), _))
      .WillOnce(RecordLocalChangeMetadata());
  EXPECT_CALL(*mock_processor_, Put("e:" + entry_uuid, _, _)).Times(0);
  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       std::move(server_records))
                   .has_value());

  EXPECT_EQ(GetConversationTitle("conv"), "Renamed");
  EXPECT_EQ(remote_changes_applied_count_, 0);
  syncer::MetadataBatch saved;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&saved));
  EXPECT_THAT(saved.GetAllMetadata(),
              ElementsAre(Pair("c:conv", HasServerVersion(1))));
}

// An entry edited while AI Chat sync was off keeps its edit through a
// first-sync merge, with no revision added, and is sent. The conversation's
// metadata, which the server already holds at the same time, isn't sent again.
TEST_F(AIChatSyncBridgeTest, MergeKeepsEntryEditTheServerLacks) {
  const std::string entry_uuid = AddTestConversation("conv", "Title");
  CreateBridge();
  syncer::EntityChangeList server_records =
      GetLocalRecordsAsRemoteChanges({"c:conv", "e:" + entry_uuid});

  // An edit is saved as the entry again, with the edit as its revision.
  task_environment_.FastForwardBy(base::Minutes(1));
  auto archive = db_->GetConversationData("conv");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  auto edited = archive->entries[0]->Clone();
  auto edit = edited->Clone();
  edit->uuid = "edit-1";
  edit->text = "Hello, edited";
  edit->created_time = base::Time::Now();
  edited->edits.emplace();
  edited->edits->push_back(std::move(edit));
  ASSERT_TRUE(db_->DeleteConversationEntry(entry_uuid));
  ASSERT_TRUE(db_->AddConversationEntry("conv", std::move(edited)));

  EXPECT_CALL(*mock_processor_,
              Put("e:" + entry_uuid, HasModifiedTime(base::Time::Now()), _));
  EXPECT_CALL(*mock_processor_, Put("c:conv", _, _)).Times(0);
  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       std::move(server_records))
                   .has_value());

  archive = db_->GetConversationData("conv");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  ASSERT_TRUE(archive->entries[0]->edits);
  ASSERT_EQ(archive->entries[0]->edits->size(), 1u);
  EXPECT_EQ(archive->entries[0]->edits->at(0)->text, "Hello, edited");
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// Associated content cleared while AI Chat sync was off, by clearing browsing
// data, stays cleared through a first-sync merge, and the entry is sent. The
// conversation's metadata isn't sent again.
TEST_F(AIChatSyncBridgeTest, MergeKeepsAssociatedContentClearTheServerLacks) {
  auto conversation = mojom::Conversation::New();
  conversation->uuid = "conv";
  conversation->title = "Title";
  conversation->associated_content.push_back(mojom::AssociatedContent::New(
      "content", mojom::ContentType::PageContent, "Page title", 1,
      GURL("https://example.com"), 100, "entry", false));
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->text = "Hello";
  entry->created_time = base::Time::Now();
  ASSERT_TRUE(db_->AddConversation(std::move(conversation), {"Page text"},
                                   std::move(entry)));
  CreateBridge();
  syncer::EntityChangeList server_records =
      GetLocalRecordsAsRemoteChanges({"c:conv", "e:entry"});
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(db_->DeleteAssociatedWebContent(std::nullopt, std::nullopt));

  EXPECT_CALL(*mock_processor_,
              Put("e:entry", HasModifiedTime(base::Time::Now()), _));
  EXPECT_CALL(*mock_processor_, Put("c:conv", _, _)).Times(0);
  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       std::move(server_records))
                   .has_value());

  // Clearing leaves no page text, url or title.
  auto archive = db_->GetConversationData("conv");
  ASSERT_TRUE(archive);
  EXPECT_TRUE(archive->associated_content.empty());
  auto conversations = db_->GetAllConversations();
  ASSERT_EQ(conversations.size(), 1u);
  ASSERT_EQ(conversations[0]->associated_content.size(), 1u);
  EXPECT_EQ(conversations[0]->associated_content[0]->url, GURL());
  EXPECT_EQ(conversations[0]->associated_content[0]->title, "");
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// An entry that grew past the size limit can't be sent, so another device may
// still edit its older version. That edit is older than the local entry, which
// is kept. It still can't be sent, so nothing is Put, and it stays tracked.
TEST_F(AIChatSyncBridgeTest, ApplyKeepsNewerEntryTooLargeToCommit) {
  const base::Time remote_time = base::Time::Now();
  task_environment_.FastForwardBy(base::Minutes(1));
  CreateBridge();
  ASSERT_NO_FATAL_FAILURE(AddEntryTooLargeToCommit("conv", "entry-1"));
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "e:entry-1",
      WithModifiedTime(MakeRemoteEntry("conv", "entry-1", "From remote"),
                       remote_time)));

  EXPECT_CALL(*mock_processor_, Put).Times(0);
  EXPECT_CALL(*mock_processor_, UntrackEntityForStorageKey).Times(0);
  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());
  RunPostedTasks();

  auto archive = db_->GetConversationData("conv");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  EXPECT_EQ(archive->entries[0]->text, "Hello");
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// A first-sync merge of this device's own records, as when AI Chat sync is
// turned off and on again, finds the same times, so it writes nothing, sends
// nothing and names no conversation. Any write would fail on this database.
TEST_F(AIChatSyncBridgeTest, MergeOfSameVersionsWritesNothing) {
  CreateBridgeWithFailingDatabase(0);
  const std::string entry_uuid = AddTestConversation("conv", "Title");
  syncer::EntityChangeList own_records =
      GetLocalRecordsAsRemoteChanges({"c:conv", "e:" + entry_uuid});
  ASSERT_EQ(own_records.size(), 2u);

  EXPECT_CALL(*mock_processor_, Put).Times(0);
  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       std::move(own_records))
                   .has_value());
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// Rows saved before records carried a time have none, and new entries don't
// give their conversation one, so such a conversation can stay untimed. Every
// change sync compares moves the time, so two untimed copies are the same
// version: a merge of this device's own untimed records writes nothing and
// names no conversation. Any write after the two that store the records would
// fail on this database.
TEST_F(AIChatSyncBridgeTest, MergeOfUntimedSameVersionsWritesNothing) {
  CreateBridgeWithFailingDatabase(2);
  auto conversation = mojom::Conversation::New();
  conversation->uuid = "conv";
  conversation->title = "Title";
  ASSERT_TRUE(db_->ApplyRemoteConversationMetadata(std::move(conversation),
                                                   base::Time()));
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->text = "Hello";
  entry->created_time = base::Time::Now();
  ASSERT_TRUE(
      db_->ApplyRemoteEntry("conv", std::move(entry), base::Time(), {}, {}));
  syncer::EntityChangeList own_records =
      GetLocalRecordsAsRemoteChanges({"c:conv", "e:entry"});
  ASSERT_EQ(own_records.size(), 2u);

  EXPECT_CALL(*mock_processor_, Put).Times(0);
  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       std::move(own_records))
                   .has_value());
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// A record changed on another device after this device's copy is written
// over it, and nothing is sent back.
TEST_F(AIChatSyncBridgeTest, ApplyWritesNewerRemoteRecord) {
  const std::string entry_uuid = AddTestConversation("conv", "Title");
  CreateBridge();
  task_environment_.FastForwardBy(base::Minutes(1));
  const base::Time remote_time = base::Time::Now();
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:conv",
      WithModifiedTime(MakeRemoteConversation("conv", "Remote"), remote_time)));
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "e:" + entry_uuid,
      WithModifiedTime(MakeRemoteEntry("conv", entry_uuid, "From remote"),
                       remote_time)));

  EXPECT_CALL(*mock_processor_, Put).Times(0);
  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());
  RunPostedTasks();

  EXPECT_EQ(GetConversationTitle("conv"), "Remote");
  auto archive = db_->GetConversationData("conv");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  EXPECT_EQ(archive->entries[0]->text, "From remote");
  EXPECT_THAT(last_affected_conversation_uuids_, ElementsAre("conv"));
}

// A record without a time, as from a local copy saved before records carried
// one, counts as the oldest. It loses to a local copy that has a time, which
// is sent instead.
TEST_F(AIChatSyncBridgeTest, UntimedRecordLosesToTimedLocalCopy) {
  AddTestConversation("conv", "Local");
  CreateBridge();
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:conv", MakeRemoteConversation("conv", "Remote")));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());
  EXPECT_CALL(*mock_processor_, Put("c:conv", _, _));
  RunPostedTasks();

  EXPECT_EQ(GetConversationTitle("conv"), "Local");
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// The notification names only the conversations a batch wrote to. A record no
// newer than the local copy writes nothing, so its conversation isn't named.
TEST_F(AIChatSyncBridgeTest, NotifiesOnlyConversationsWithNewerRecords) {
  AddTestConversation("older-conv", "Local");
  AddTestConversation("same-conv", "Local");
  AddTestConversation("newer-conv", "Local");
  const base::Time local_time = base::Time::Now();
  CreateBridge();
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:older-conv",
      WithModifiedTime(MakeRemoteConversation("older-conv", "Remote"),
                       local_time - base::Minutes(1))));
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:same-conv",
      WithModifiedTime(MakeRemoteConversation("same-conv", "Remote"),
                       local_time)));
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:newer-conv",
      WithModifiedTime(MakeRemoteConversation("newer-conv", "Remote"),
                       local_time + base::Minutes(1))));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  EXPECT_EQ(remote_changes_applied_count_, 1);
  EXPECT_THAT(last_affected_conversation_uuids_, ElementsAre("newer-conv"));
}

// The processor counts a Put() made while it applies remote changes, and means
// to forbid it. So a local copy ApplyIncrementalSyncChanges() keeps is sent,
// with its own time, once that has returned, and its sync metadata is saved
// straight away.
TEST_F(AIChatSyncBridgeTest, KeptLocalCopyIsSentAfterTheIncrementalApply) {
  const std::string entry_uuid = AddTestConversation("conv", "Local");
  const base::Time local_time = base::Time::Now();
  CreateBridge();
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:conv", WithModifiedTime(MakeRemoteConversation("conv", "Remote"),
                                 local_time - base::Minutes(1))));
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "e:" + entry_uuid,
      WithModifiedTime(MakeRemoteEntry("conv", entry_uuid, "From remote"),
                       local_time - base::Minutes(1))));

  EXPECT_CALL(*mock_processor_, Put).Times(0);
  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());
  testing::Mock::VerifyAndClearExpectations(mock_processor_.get());

  EXPECT_CALL(*mock_processor_, Put("c:conv", HasModifiedTime(local_time), _))
      .WillOnce(RecordLocalChangeMetadata());
  EXPECT_CALL(*mock_processor_,
              Put("e:" + entry_uuid, HasModifiedTime(local_time), _))
      .WillOnce(RecordLocalChangeMetadata());
  RunPostedTasks();

  syncer::MetadataBatch saved;
  ASSERT_TRUE(db_->GetAllSyncMetadata(&saved));
  EXPECT_THAT(saved.GetAllMetadata(),
              ElementsAre(Pair("c:conv", _), Pair("e:" + entry_uuid, _)));
}

// A kept local copy is sent only if sync can still take it when its task
// runs: not once the database is detached, or sync has stopped tracking.
TEST_F(AIChatSyncBridgeTest, KeptLocalCopyIsNotSentOnceSyncCannotTakeIt) {
  AddTestConversation("conv", "Local");
  const base::Time older = base::Time::Now() - base::Minutes(1);
  CreateBridge();
  auto make_changes = [&] {
    syncer::EntityChangeList changes;
    changes.push_back(syncer::EntityChange::CreateUpdate(
        "c:conv",
        WithModifiedTime(MakeRemoteConversation("conv", "Remote"), older)));
    return changes;
  };
  EXPECT_CALL(*mock_processor_, Put).Times(0);

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), make_changes())
                   .has_value());
  bridge_->ClearDatabase();
  RunPostedTasks();

  bridge_->SetDatabase(db_.get());
  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), make_changes())
                   .has_value());
  ON_CALL(*mock_processor_, IsTrackingMetadata())
      .WillByDefault(testing::Return(false));
  RunPostedTasks();
}

// A record changed locally between ApplyIncrementalSyncChanges() and the task
// that sends its kept copy is sent by that change, with its new time, and not
// again by the task with the time it was kept with.
TEST_F(AIChatSyncBridgeTest, KeptLocalCopyChangedLocallySinceIsNotSentAgain) {
  AddTestConversation("conv", "Local");
  CreateBridge();
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:conv", WithModifiedTime(MakeRemoteConversation("conv", "Remote"),
                                 base::Time::Now() - base::Minutes(1))));
  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_TRUE(db_->UpdateConversationTitle("conv", "Renamed"));
  EXPECT_CALL(*mock_processor_,
              Put("c:conv", HasModifiedTime(base::Time::Now()), _));
  bridge_->OnConversationModified("conv");
  RunPostedTasks();
}

// A newer remote record written between ApplyIncrementalSyncChanges() and the
// task that sends a kept copy of it replaces that copy, so nothing is sent.
TEST_F(AIChatSyncBridgeTest, KeptLocalCopyReplacedByNewerRecordIsNotSent) {
  AddTestConversation("conv", "Local");
  const base::Time local_time = base::Time::Now();
  CreateBridge();
  syncer::EntityChangeList older;
  older.push_back(syncer::EntityChange::CreateUpdate(
      "c:conv", WithModifiedTime(MakeRemoteConversation("conv", "Older"),
                                 local_time - base::Minutes(1))));
  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(older))
                   .has_value());
  syncer::EntityChangeList newer;
  newer.push_back(syncer::EntityChange::CreateUpdate(
      "c:conv", WithModifiedTime(MakeRemoteConversation("conv", "Newer"),
                                 local_time + base::Minutes(1))));
  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(newer))
                   .has_value());

  EXPECT_CALL(*mock_processor_, Put).Times(0);
  RunPostedTasks();
  EXPECT_EQ(GetConversationTitle("conv"), "Newer");
}

// A batch that fails at a record before the task that sends its kept copy
// runs leaves the processor holding that batch's version of the record, so
// the kept copy is skipped like any other local update to it.
TEST_F(AIChatSyncBridgeTest, KeptLocalCopyIsNotSentAfterALaterBatchFailsAtIt) {
  CreateBridgeWithFailingDatabase(0);
  AddTestConversation("conv", "Local");
  const base::Time local_time = base::Time::Now();
  syncer::EntityChangeList older;
  older.push_back(syncer::EntityChange::CreateUpdate(
      "c:conv", WithModifiedTime(MakeRemoteConversation("conv", "Older"),
                                 local_time - base::Minutes(1))));
  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(older))
                   .has_value());
  syncer::EntityChangeList newer;
  newer.push_back(syncer::EntityChange::CreateUpdate(
      "c:conv", WithModifiedTime(MakeRemoteConversation("conv", "Newer"),
                                 local_time + base::Minutes(1))));
  ASSERT_TRUE(bridge_
                  ->ApplyIncrementalSyncChanges(
                      bridge_->CreateMetadataChangeList(), std::move(newer))
                  .has_value());

  EXPECT_CALL(*mock_processor_, Put).Times(0);
  RunPostedTasks();
}

// A batch that fails downloads again on the next start, which compares its
// records again. So a local copy kept before the failure isn't sent.
TEST_F(AIChatSyncBridgeTest, KeptLocalCopyIsNotSentWhenTheBatchFails) {
  CreateBridgeWithFailingDatabase(0);
  AddTestConversation("local-conv", "Local");
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:local-conv",
      WithModifiedTime(MakeRemoteConversation("local-conv", "Remote"),
                       base::Time::Now() - base::Minutes(1))));
  changes.push_back(syncer::EntityChange::CreateAdd(
      "c:remote-conv", MakeRemoteConversation("remote-conv", "Remote")));

  EXPECT_CALL(*mock_processor_, Put).Times(0);
  ASSERT_TRUE(bridge_
                  ->ApplyIncrementalSyncChanges(
                      bridge_->CreateMetadataChangeList(), std::move(changes))
                  .has_value());
  RunPostedTasks();
}

// The processor calls ResolveConflict() when a remote change arrives for a
// record this device has changed but not yet committed. kUseRemote drops that
// commit and passes the record to ApplyIncrementalSyncChanges(), so it's
// returned only when that would write the record.
TEST_F(AIChatSyncBridgeTest, ResolveConflictKeepsTheNewerCopy) {
  const std::string entry_uuid = AddTestConversation("conv", "Title");
  const std::string entry_key = "e:" + entry_uuid;
  const base::Time local_time = base::Time::Now();
  const base::Time older = local_time - base::Minutes(1);
  const base::Time newer = local_time + base::Minutes(1);
  auto untimed = mojom::Conversation::New();
  untimed->uuid = "untimed-conv";
  ASSERT_TRUE(
      db_->ApplyRemoteConversationMetadata(std::move(untimed), base::Time()));
  CreateBridge();
  auto conversation = [](base::Time time) {
    return WithModifiedTime(MakeRemoteConversation("conv", "Remote"), time);
  };
  auto entry = [&](base::Time time) {
    return WithModifiedTime(MakeRemoteEntry("conv", entry_uuid, "Remote"),
                            time);
  };

  EXPECT_EQ(bridge_->ResolveConflict("c:conv", conversation(older)),
            syncer::ConflictResolution::kUseLocal);
  EXPECT_EQ(bridge_->ResolveConflict("c:conv", conversation(local_time)),
            syncer::ConflictResolution::kUseLocal);
  EXPECT_EQ(bridge_->ResolveConflict("c:conv", conversation(base::Time())),
            syncer::ConflictResolution::kUseLocal);
  EXPECT_EQ(bridge_->ResolveConflict("c:conv", conversation(newer)),
            syncer::ConflictResolution::kUseRemote);

  EXPECT_EQ(bridge_->ResolveConflict(entry_key, entry(older)),
            syncer::ConflictResolution::kUseLocal);
  EXPECT_EQ(bridge_->ResolveConflict(entry_key, entry(local_time)),
            syncer::ConflictResolution::kUseLocal);
  EXPECT_EQ(bridge_->ResolveConflict(entry_key, entry(base::Time())),
            syncer::ConflictResolution::kUseLocal);
  EXPECT_EQ(bridge_->ResolveConflict(entry_key, entry(newer)),
            syncer::ConflictResolution::kUseRemote);

  // Two untimed copies are the same version.
  EXPECT_EQ(
      bridge_->ResolveConflict(
          "c:untimed-conv", MakeRemoteConversation("untimed-conv", "Remote")),
      syncer::ConflictResolution::kUseLocal);
  // A remote delete carries no time, and loses to the local change.
  EXPECT_EQ(bridge_->ResolveConflict("c:conv", syncer::EntityData()),
            syncer::ConflictResolution::kUseLocal);
  // With no local copy, the remote one is applied.
  EXPECT_EQ(bridge_->ResolveConflict("c:other",
                                     MakeRemoteConversation("other", "Remote")),
            syncer::ConflictResolution::kUseRemote);
}

// ResolveConflict() can't report a failed read of the local time, so it
// passes the record on to ApplyIncrementalSyncChanges(), which reads again. The
// remote copy is older, so a successful read would have kept the local one.
TEST_F(AIChatSyncBridgeTest,
       ResolveConflictUsesRemoteWhenLocalTimeCannotBeRead) {
  CreateBridgeWithUnreadableTimes();
  AddTestConversation("conv", "Title");

  EXPECT_EQ(
      bridge_->ResolveConflict(
          "c:conv", WithModifiedTime(MakeRemoteConversation("conv", "Remote"),
                                     base::Time::Now() - base::Minutes(1))),
      syncer::ConflictResolution::kUseRemote);
}

// The local copy's time decides whether a record is written over it, so a
// failed read of that time fails the batch rather than writing.
TEST_F(AIChatSyncBridgeTest,
       ApplyIncrementalSyncChangesFailsWhenLocalTimeCannotBeRead) {
  CreateBridgeWithUnreadableTimes();
  AddTestConversation("conv", "Title");
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "c:conv", WithModifiedTime(MakeRemoteConversation("conv", "Remote"),
                                 base::Time::Now() + base::Minutes(1))));

  auto error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(), std::move(changes));
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);
  EXPECT_EQ(GetConversationTitle("conv"), "Title");
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

TEST_F(AIChatSyncBridgeTest, MergeFullSyncDataFailsWhenLocalTimeCannotBeRead) {
  CreateBridgeWithUnreadableTimes();
  const std::string entry_uuid = AddTestConversation("conv", "Title");
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateAdd(
      "e:" + entry_uuid,
      WithModifiedTime(MakeRemoteEntry("conv", entry_uuid, "From remote"),
                       base::Time::Now() + base::Minutes(1))));

  auto error = bridge_->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                          std::move(changes));
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);
  auto archive = db_->GetConversationData("conv");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->entries.size(), 1u);
  EXPECT_EQ(archive->entries[0]->text, "Hello");
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// An entry can arrive before its conversation's metadata record, in the same
// batch or a separate one, and is then held in a hidden stub conversation. A
// stub counts as no local copy, so the metadata record fills it in whatever
// time the record carries, including none.
TEST_F(AIChatSyncBridgeTest, MetadataRecordFillsInStubWhateverItsTime) {
  CreateBridge();
  const base::Time remote_time = base::Time::Now() - base::Hours(1);

  syncer::EntityChangeList same_batch;
  same_batch.push_back(syncer::EntityChange::CreateAdd(
      "e:entry-a",
      WithModifiedTime(MakeRemoteEntry("conv-a", "entry-a", "Hello"),
                       remote_time)));
  same_batch.push_back(syncer::EntityChange::CreateAdd(
      "c:conv-a", WithModifiedTime(MakeRemoteConversation("conv-a", "Title A"),
                                   remote_time)));
  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       std::move(same_batch))
                   .has_value());

  for (const std::string suffix : {"b", "c"}) {
    // The records for conversation c carry no time.
    const base::Time time = suffix == "b" ? remote_time : base::Time();
    syncer::EntityChangeList entry_batch;
    entry_batch.push_back(syncer::EntityChange::CreateAdd(
        "e:entry-" + suffix,
        WithModifiedTime(
            MakeRemoteEntry("conv-" + suffix, "entry-" + suffix, "Hello"),
            time)));
    ASSERT_FALSE(
        bridge_
            ->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                          std::move(entry_batch))
            .has_value());
    syncer::EntityChangeList metadata_batch;
    metadata_batch.push_back(syncer::EntityChange::CreateAdd(
        "c:conv-" + suffix,
        WithModifiedTime(
            MakeRemoteConversation("conv-" + suffix, "Title " + suffix),
            time)));
    ASSERT_FALSE(
        bridge_
            ->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                          std::move(metadata_batch))
            .has_value());
  }

  EXPECT_EQ(GetConversationTitle("conv-a"), "Title A");
  EXPECT_EQ(GetConversationTitle("conv-b"), "Title b");
  EXPECT_EQ(GetConversationTitle("conv-c"), "Title c");
  EXPECT_THAT(db_->GetConversationModifiedTime("conv-a"),
              IsModifiedAt(remote_time));
  EXPECT_THAT(db_->GetConversationModifiedTime("conv-b"),
              IsModifiedAt(remote_time));
  EXPECT_THAT(db_->GetConversationModifiedTime("conv-c"),
              IsModifiedAt(base::Time()));
}

// Sync sends an edited entry as its latest revision alone, so writing a newer
// record from another device as it is would delete this device's revisions,
// including the original text. The record's content is added as a new latest
// revision instead, dated when it changed on the other device.
TEST_F(AIChatSyncBridgeTest, ApplyKeepsLocalEditHistory) {
  ASSERT_NO_FATAL_FAILURE(SeedEditedEntry("conv", "entry"));
  CreateBridge();
  task_environment_.FastForwardBy(base::Minutes(1));
  const base::Time remote_time = base::Time::Now();
  task_environment_.FastForwardBy(base::Minutes(1));
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "e:entry",
      WithModifiedTime(MakeRemoteEntry("conv", "entry", "Revised elsewhere"),
                       remote_time)));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  EXPECT_THAT(
      GetEntryAndRevisionTexts("conv"),
      ElementsAre("Original question", "Edited question", "Revised elsewhere"));
  auto entry = db_->GetConversationEntryWithEdits("entry");
  ASSERT_TRUE(entry && *entry && (*entry)->edits);
  ASSERT_EQ((*entry)->edits->size(), 2u);
  EXPECT_EQ((*entry)->edits->at(0)->uuid, "edit-1");
  EXPECT_EQ((*entry)->edits->at(1)->created_time, remote_time);
  EXPECT_THAT(db_->GetEntryModifiedTime("entry"), IsModifiedAt(remote_time));
  EXPECT_THAT(last_affected_conversation_uuids_, ElementsAre("conv"));
}

// An entry saved before records carried a time has none, so a record with any
// time is written over it, even a time before the entry's latest revision. The
// record's revision is then dated just after that revision, so that it's the
// latest.
TEST_F(AIChatSyncBridgeTest, ApplyDatesRevisionAfterLocalOnes) {
  ASSERT_NO_FATAL_FAILURE(SeedEditedEntry("conv", "entry"));
  auto local = db_->GetConversationEntryWithEdits("entry");
  ASSERT_TRUE(local && *local && (*local)->edits);
  const base::Time local_revision_time = (*local)->edits->back()->created_time;
  ASSERT_TRUE(
      db_->ApplyRemoteEntry("conv", std::move(*local), base::Time(), {}, {}));
  ASSERT_THAT(db_->GetEntryModifiedTime("entry"), IsModifiedAt(base::Time()));
  CreateBridge();
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "e:entry",
      WithModifiedTime(MakeRemoteEntry("conv", "entry", "Revised elsewhere"),
                       local_revision_time - base::Seconds(30))));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  EXPECT_THAT(
      GetEntryAndRevisionTexts("conv"),
      ElementsAre("Original question", "Edited question", "Revised elsewhere"));
  auto entry = db_->GetConversationEntryWithEdits("entry");
  ASSERT_TRUE(entry && *entry && (*entry)->edits);
  ASSERT_EQ((*entry)->edits->size(), 2u);
  EXPECT_EQ((*entry)->edits->at(1)->created_time,
            local_revision_time + base::Microseconds(1));
}

// A first-sync merge, as when AI Chat sync is turned off and on again, can
// bring a record that's newer than this device's entry but has the content of
// its latest revision, as when another device changed only the entry's
// associated content. The revisions are kept, and none is added.
TEST_F(AIChatSyncBridgeTest, MergeFullSyncDataDoesNotDuplicateLocalEdits) {
  ASSERT_NO_FATAL_FAILURE(SeedEditedEntry("conv", "entry"));
  CreateBridge();
  auto local = db_->GetConversationEntryWithEdits("entry");
  ASSERT_TRUE(local && *local);
  task_environment_.FastForwardBy(base::Minutes(2));
  syncer::EntityData data;
  *data.specifics.mutable_ai_chat_conversation() =
      EntryToSpecifics("conv", **local, base::Time::Now(), {});
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateAdd("e:entry", std::move(data)));

  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       std::move(changes))
                   .has_value());

  EXPECT_THAT(GetEntryAndRevisionTexts("conv"),
              ElementsAre("Original question", "Edited question"));
  // The record was written, with its time.
  EXPECT_THAT(db_->GetEntryModifiedTime("entry"),
              IsModifiedAt(base::Time::Now()));
}

// Without the snapshot of which entries have edit revisions, an entry's
// revisions would look absent and the write would delete them, so the batch
// fails instead.
TEST_F(AIChatSyncBridgeTest, ApplyFailsWhenEditRevisionsCannotBeRead) {
  db_.reset();
  db_ = std::make_unique<UnreadableEditRevisionsAIChatDatabase>(db_file_path(),
                                                                encryptor_);
  CreateBridge();
  ASSERT_NO_FATAL_FAILURE(SeedEditedEntry("conv", "entry"));
  task_environment_.FastForwardBy(base::Minutes(2));
  auto make_changes = [] {
    syncer::EntityChangeList changes;
    changes.push_back(syncer::EntityChange::CreateAdd(
        "e:entry",
        WithModifiedTime(MakeRemoteEntry("conv", "entry", "Revised elsewhere"),
                         base::Time::Now())));
    return changes;
  };

  auto merge_error = bridge_->MergeFullSyncData(
      bridge_->CreateMetadataChangeList(), make_changes());
  ASSERT_TRUE(merge_error.has_value());
  EXPECT_EQ(merge_error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);
  auto apply_error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(), make_changes());
  ASSERT_TRUE(apply_error.has_value());
  EXPECT_EQ(apply_error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);

  EXPECT_THAT(GetEntryAndRevisionTexts("conv"),
              ElementsAre("Original question", "Edited question"));
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// The snapshot names the entry, but the entry with its revisions can't be
// read. Writing the record as it is would delete them, so the batch fails.
TEST_F(AIChatSyncBridgeTest, ApplyFailsWhenEditedEntryCannotBeRead) {
  db_.reset();
  db_ = std::make_unique<UnreadableEntryAIChatDatabase>(db_file_path(),
                                                        encryptor_);
  CreateBridge();
  ASSERT_NO_FATAL_FAILURE(SeedEditedEntry("conv", "entry"));
  task_environment_.FastForwardBy(base::Minutes(2));
  syncer::EntityChangeList changes;
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "e:entry",
      WithModifiedTime(MakeRemoteEntry("conv", "entry", "Revised elsewhere"),
                       base::Time::Now())));

  auto error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(), std::move(changes));
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);
  EXPECT_THAT(GetEntryAndRevisionTexts("conv"),
              ElementsAre("Original question", "Edited question"));
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// The processor calls ApplyIncrementalSyncChanges() with no changes after
// every commit, and a first sync can bring none. Neither reads which entries
// have edit revisions.
TEST_F(AIChatSyncBridgeTest, ApplyOfEmptyBatchDoesNotReadEditRevisions) {
  db_.reset();
  auto database = std::make_unique<UnreadableEditRevisionsAIChatDatabase>(
      db_file_path(), encryptor_);
  const UnreadableEditRevisionsAIChatDatabase* edit_revisions_database =
      database.get();
  db_ = std::move(database);
  CreateBridge();

  EXPECT_FALSE(
      bridge_
          ->ApplyIncrementalSyncChanges(bridge_->CreateMetadataChangeList(),
                                        syncer::EntityChangeList())
          .has_value());
  EXPECT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       syncer::EntityChangeList())
                   .has_value());
  EXPECT_EQ(edit_revisions_database->read_count(), 0);
}

// A batch can hold a tombstone for a conversation, then a record for one of
// its edited entries, as when another device changed the entry while a third
// deleted the conversation. The batch's snapshot still names the entry, but
// the tombstone deleted it with its revisions, so the record is written as it
// is, into a hidden stub conversation.
TEST_F(AIChatSyncBridgeTest, ApplyWritesRecordForEditedEntryTheBatchDeleted) {
  ASSERT_NO_FATAL_FAILURE(SeedEditedEntry("conv", "entry"));
  CreateBridge();
  task_environment_.FastForwardBy(base::Minutes(1));
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateDelete("c:conv", syncer::EntityData()));
  changes.push_back(syncer::EntityChange::CreateUpdate(
      "e:entry",
      WithModifiedTime(MakeRemoteEntry("conv", "entry", "Revised elsewhere"),
                       base::Time::Now())));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  EXPECT_THAT(GetEntryAndRevisionTexts("conv"),
              ElementsAre("Revised elsewhere"));
  EXPECT_EQ(GetConversationTitle("conv"), std::nullopt);
}

// Keeping the edit history writes the entry again from this device's copy, so
// a revision's events must come through it. Workspace content, which sync
// never carries, is kept by the database's write itself, and is checked here
// so that keeping the history can't lose it.
TEST_F(AIChatSyncBridgeTest, ApplyKeepsEditedEntryEventsAndWorkspaceContent) {
  std::vector<mojom::ConversationEntryEventPtr> revision_events;
  revision_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("Edited answer")));
  revision_events.push_back(
      mojom::ConversationEntryEvent::NewSearchQueriesEvent(
          mojom::SearchQueriesEvent::New(std::vector<std::string>{"query"})));
  std::vector<mojom::AssociatedContentPtr> content;
  content.push_back(mojom::AssociatedContent::New(
      "workspace", mojom::ContentType::Workspace, "Workspace", 1,
      GURL("https://workspace.test"), 100, "entry", false));
  ASSERT_NO_FATAL_FAILURE(SeedEditedAnswer(
      std::move(revision_events), std::move(content), {"Workspace text"}));
  CreateBridge();
  task_environment_.FastForwardBy(base::Minutes(1));
  syncer::EntityData data =
      WithModifiedTime(MakeRemoteEntry("conv", "entry", ""), base::Time::Now());
  auto* remote = data.specifics.mutable_ai_chat_conversation()->mutable_entry();
  remote->set_character_type(
      std::to_underlying(mojom::CharacterType::ASSISTANT));
  remote->set_action_type(std::to_underlying(mojom::ActionType::RESPONSE));
  remote->add_events()->mutable_completion()->set_raw("Revised elsewhere");
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateUpdate("e:entry", std::move(data)));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  auto entry = db_->GetConversationEntryWithEdits("entry");
  ASSERT_TRUE(entry && *entry && (*entry)->events && (*entry)->edits);
  ASSERT_EQ((*entry)->events->size(), 1u);
  EXPECT_EQ((*entry)->events->at(0)->get_completion_event()->completion,
            "Original answer");
  ASSERT_EQ((*entry)->edits->size(), 2u);
  const auto& kept_events = (*entry)->edits->at(0)->events;
  ASSERT_TRUE(kept_events);
  ASSERT_EQ(kept_events->size(), 2u);
  EXPECT_EQ(kept_events->at(0)->get_completion_event()->completion,
            "Edited answer");
  EXPECT_THAT(kept_events->at(1)->get_search_queries_event()->search_queries,
              ElementsAre("query"));
  const auto& added_events = (*entry)->edits->at(1)->events;
  ASSERT_TRUE(added_events);
  ASSERT_EQ(added_events->size(), 1u);
  EXPECT_EQ(added_events->at(0)->get_completion_event()->completion,
            "Revised elsewhere");

  auto conversations = db_->GetAllConversations();
  ASSERT_EQ(conversations.size(), 1u);
  ASSERT_EQ(conversations[0]->associated_content.size(), 1u);
  EXPECT_EQ(conversations[0]->associated_content[0]->content_type,
            mojom::ContentType::Workspace);
  auto archive = db_->GetConversationData("conv");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->associated_content.size(), 1u);
  EXPECT_EQ(archive->associated_content[0]->content, "Workspace text");
}

// A field sync doesn't carry, such as a tool artifact's id, is in this
// device's latest revision but never in a record. So it isn't a difference: a
// newer record with that revision's content adds no revision, and the local
// revision keeps the field.
TEST_F(AIChatSyncBridgeTest, ApplyAddsNoRevisionForFieldsSyncDoesNotCarry) {
  std::vector<mojom::ToolArtifactPtr> artifacts;
  artifacts.push_back(
      mojom::ToolArtifact::New("artifact-1", "chart", R"({"x": 1})"));
  std::vector<mojom::ConversationEntryEventPtr> revision_events;
  revision_events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          "chart_tool", "tool-1", R"({"a": 1})", std::nullopt,
          std::move(artifacts), nullptr, false)));
  ASSERT_NO_FATAL_FAILURE(SeedEditedAnswer(std::move(revision_events), {}, {}));
  CreateBridge();
  auto local = db_->GetConversationEntryWithEdits("entry");
  ASSERT_TRUE(local && *local);
  task_environment_.FastForwardBy(base::Minutes(1));
  syncer::EntityData data;
  *data.specifics.mutable_ai_chat_conversation() =
      EntryToSpecifics("conv", **local, base::Time::Now(), {});
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateUpdate("e:entry", std::move(data)));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  // The record was written, with its time.
  EXPECT_THAT(db_->GetEntryModifiedTime("entry"),
              IsModifiedAt(base::Time::Now()));
  auto entry = db_->GetConversationEntryWithEdits("entry");
  ASSERT_TRUE(entry && *entry && (*entry)->edits);
  ASSERT_EQ((*entry)->edits->size(), 1u);
  const auto& events = (*entry)->edits->at(0)->events;
  ASSERT_TRUE(events);
  ASSERT_EQ(events->size(), 1u);
  const auto& tool_use = events->at(0)->get_tool_use_event();
  ASSERT_TRUE(tool_use->artifacts);
  ASSERT_EQ(tool_use->artifacts->size(), 1u);
  EXPECT_EQ(tool_use->artifacts->at(0)->id, "artifact-1");
}

// A sender that omits a value to fit the size limit sends a hash of it
// instead. This device's entry holds a completion with that hash, so the
// newer record is written with it rather than with a blank completion.
TEST_F(AIChatSyncBridgeTest, ApplyRestoresOmittedCompletion) {
  ASSERT_NO_FATAL_FAILURE(SeedCompletion("The answer"));
  CreateBridge();
  task_environment_.FastForwardBy(base::Minutes(1));

  ASSERT_FALSE(
      bridge_
          ->ApplyIncrementalSyncChanges(
              bridge_->CreateMetadataChangeList(),
              MakeOmittedCompletionUpdate("The answer", base::Time::Now()))
          .has_value());

  EXPECT_THAT(GetCompletions("conv"), ElementsAre("The answer"));
  // The record was written, with its time.
  EXPECT_THAT(db_->GetEntryModifiedTime("entry"),
              IsModifiedAt(base::Time::Now()));
}

// The sender's completion differs from this device's, so the hash it sent
// matches nothing here. This device's completion isn't put in its place, and
// the omitted completion decodes as blank.
TEST_F(AIChatSyncBridgeTest, ApplyDoesNotRestoreMismatchedCompletion) {
  ASSERT_NO_FATAL_FAILURE(SeedCompletion("This device's answer"));
  CreateBridge();
  task_environment_.FastForwardBy(base::Minutes(1));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(),
                       MakeOmittedCompletionUpdate("The sender's answer",
                                                   base::Time::Now()))
                   .has_value());

  EXPECT_THAT(GetCompletions("conv"), ElementsAre(""));
}

// A sender omits uploaded file bytes first, largest first, and then page text.
// Page text is put back from the text of this device's associated content with
// the same hash.
TEST_F(AIChatSyncBridgeTest, ApplyRestoresOmittedPageText) {
  auto conversation = mojom::Conversation::New();
  conversation->uuid = "conv";
  conversation->title = "Local";
  conversation->associated_content.push_back(mojom::AssociatedContent::New(
      "content", mojom::ContentType::PageContent, "Remote page", 1,
      GURL("https://example.com/remote"), 100, "entry", false));
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->text = "Hello";
  entry->created_time = base::Time::Now();
  ASSERT_TRUE(db_->AddConversation(std::move(conversation), {"Page text"},
                                   std::move(entry)));
  CreateBridge();
  task_environment_.FastForwardBy(base::Minutes(1));
  syncer::EntityData data = WithModifiedTime(
      MakeRemoteEntry("conv", "entry", "Hello"), base::Time::Now());
  AddRemoteAssociatedContent(data, "content");
  auto* page_text = data.specifics.mutable_ai_chat_conversation()
                        ->mutable_entry()
                        ->mutable_associated_content(0)
                        ->mutable_last_contents();
  WriteCompressibleString("Page text", page_text);
  OmitCompressibleString(page_text);
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateUpdate("e:entry", std::move(data)));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  auto archive = db_->GetConversationData("conv");
  ASSERT_TRUE(archive);
  ASSERT_EQ(archive->associated_content.size(), 1u);
  EXPECT_EQ(archive->associated_content[0]->content_uuid, "content");
  EXPECT_EQ(archive->associated_content[0]->content, "Page text");
  EXPECT_THAT(db_->GetEntryModifiedTime("entry"),
              IsModifiedAt(base::Time::Now()));
}

// A sender builds an edited entry's record from its latest revision, so the
// hash of an omitted file is that of the revision's file, not of the entry's
// original one. The file is put back from the latest revision, so the record
// matches it and adds no revision.
TEST_F(AIChatSyncBridgeTest, MergeFullSyncDataRestoresOmittedFileOfLatestEdit) {
  std::vector<mojom::UploadedFilePtr> original_files;
  original_files.push_back(
      mojom::UploadedFile::New("original.png", 3, std::vector<uint8_t>{1, 2, 3},
                               mojom::UploadedFileType::kImage, std::nullopt));
  std::vector<mojom::UploadedFilePtr> revision_files;
  revision_files.push_back(
      mojom::UploadedFile::New("edit.png", 3, std::vector<uint8_t>{4, 5, 6},
                               mojom::UploadedFileType::kImage, std::nullopt));
  ASSERT_NO_FATAL_FAILURE(SeedEditedEntry(
      "conv", "entry", std::move(original_files), std::move(revision_files)));
  CreateBridge();
  auto local = db_->GetConversationEntryWithEdits("entry");
  ASSERT_TRUE(local && *local);
  task_environment_.FastForwardBy(base::Minutes(1));
  syncer::EntityData data;
  auto* specifics = data.specifics.mutable_ai_chat_conversation();
  *specifics = EntryToSpecifics("conv", **local, base::Time::Now(), {});
  ASSERT_EQ(specifics->entry().uploaded_files_size(), 1);
  OmitUploadedFileData(specifics->mutable_entry()->mutable_uploaded_files(0));
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateAdd("e:entry", std::move(data)));

  ASSERT_FALSE(bridge_
                   ->MergeFullSyncData(bridge_->CreateMetadataChangeList(),
                                       std::move(changes))
                   .has_value());

  EXPECT_THAT(db_->GetEntryModifiedTime("entry"),
              IsModifiedAt(base::Time::Now()));
  auto applied = db_->GetConversationEntryWithEdits("entry");
  ASSERT_TRUE(applied && *applied && (*applied)->edits);
  ASSERT_EQ((*applied)->edits->size(), 1u);
  const auto& files = (*applied)->edits->front()->uploaded_files;
  ASSERT_TRUE(files);
  ASSERT_EQ(files->size(), 1u);
  EXPECT_EQ(files->front()->data, std::vector<uint8_t>({4, 5, 6}));
}

// Omitted values are put back before the record is compared with the latest
// revision of an edited entry. So a record that omits a value it shares with
// that revision matches it, and adds no revision.
TEST_F(AIChatSyncBridgeTest,
       ApplyAddsNoRevisionForOmittedValueOfLatestRevision) {
  std::vector<mojom::ConversationEntryEventPtr> revision_events;
  revision_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("Edited answer")));
  ASSERT_NO_FATAL_FAILURE(SeedEditedAnswer(std::move(revision_events), {}, {}));
  CreateBridge();
  auto local = db_->GetConversationEntryWithEdits("entry");
  ASSERT_TRUE(local && *local);
  task_environment_.FastForwardBy(base::Minutes(1));
  syncer::EntityData data;
  auto* specifics = data.specifics.mutable_ai_chat_conversation();
  *specifics = EntryToSpecifics("conv", **local, base::Time::Now(), {});
  ASSERT_EQ(specifics->entry().events_size(), 1);
  OmitCompressibleString(
      specifics->mutable_entry()->mutable_events(0)->mutable_completion());
  syncer::EntityChangeList changes;
  changes.push_back(
      syncer::EntityChange::CreateUpdate("e:entry", std::move(data)));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(), std::move(changes))
                   .has_value());

  EXPECT_THAT(db_->GetEntryModifiedTime("entry"),
              IsModifiedAt(base::Time::Now()));
  auto entry = db_->GetConversationEntryWithEdits("entry");
  ASSERT_TRUE(entry && *entry && (*entry)->edits);
  ASSERT_EQ((*entry)->edits->size(), 1u);
  const auto& events = (*entry)->edits->at(0)->events;
  ASSERT_TRUE(events);
  ASSERT_EQ(events->size(), 1u);
  EXPECT_EQ(events->at(0)->get_completion_event()->completion, "Edited answer");
}

// Putting back an omitted value needs this device's entry. If it can't be
// read, the write would blank the value, so the batch fails instead.
TEST_F(AIChatSyncBridgeTest, ApplyFailsWhenEntryWithOmittedValueCannotBeRead) {
  db_.reset();
  db_ = std::make_unique<UnreadableEntryAIChatDatabase>(db_file_path(),
                                                        encryptor_);
  CreateBridge();
  ASSERT_NO_FATAL_FAILURE(SeedCompletion("The answer"));
  task_environment_.FastForwardBy(base::Minutes(1));

  auto error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(),
      MakeOmittedCompletionUpdate("The answer", base::Time::Now()));
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);
  EXPECT_THAT(GetCompletions("conv"), ElementsAre("The answer"));
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// Putting back an omitted value also needs the texts of the entry's
// associated content. If they can't be read, the batch fails.
TEST_F(AIChatSyncBridgeTest, ApplyFailsWhenPageTextsCannotBeRead) {
  db_.reset();
  db_ = std::make_unique<UnreadablePageTextsAIChatDatabase>(db_file_path(),
                                                            encryptor_);
  CreateBridge();
  ASSERT_NO_FATAL_FAILURE(SeedCompletion("The answer"));
  task_environment_.FastForwardBy(base::Minutes(1));

  auto error = bridge_->ApplyIncrementalSyncChanges(
      bridge_->CreateMetadataChangeList(),
      MakeOmittedCompletionUpdate("The answer", base::Time::Now()));
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(error->type(),
            syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);
  EXPECT_THAT(GetCompletions("conv"), ElementsAre("The answer"));
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// With no local entry there's nothing to put back, so a record that omits a
// value is written without reading this device's entry or page texts. Any such
// read would fail on this database.
TEST_F(AIChatSyncBridgeTest, ApplyOfOmittedValueWithNoLocalEntryReadsNothing) {
  db_.reset();
  db_ = std::make_unique<UnreadableEntryAIChatDatabase>(db_file_path(),
                                                        encryptor_);
  CreateBridge();

  ASSERT_FALSE(
      bridge_
          ->ApplyIncrementalSyncChanges(
              bridge_->CreateMetadataChangeList(),
              MakeOmittedCompletionUpdate("The answer", base::Time::Now()))
          .has_value());

  EXPECT_THAT(GetCompletions("conv"), ElementsAre(""));
  EXPECT_THAT(last_affected_conversation_uuids_, ElementsAre("conv"));
}

// A record with the same time as this device's entry, or an older one, isn't
// written, so an omitted value in it needs no reads. Any such read would fail
// on this database.
TEST_F(AIChatSyncBridgeTest,
       ApplyOfSameOrOlderRecordWithOmittedValueReadsNothing) {
  db_.reset();
  db_ = std::make_unique<UnreadableEntryAIChatDatabase>(db_file_path(),
                                                        encryptor_);
  CreateBridge();
  ASSERT_NO_FATAL_FAILURE(SeedCompletion("The answer"));

  for (base::Time record_time :
       {base::Time::Now(), base::Time::Now() - base::Minutes(1)}) {
    ASSERT_FALSE(
        bridge_
            ->ApplyIncrementalSyncChanges(
                bridge_->CreateMetadataChangeList(),
                MakeOmittedCompletionUpdate("Another answer", record_time))
            .has_value());
  }

  EXPECT_THAT(GetCompletions("conv"), ElementsAre("The answer"));
  EXPECT_EQ(remote_changes_applied_count_, 0);
}

// A value of at least kSyncCompressionThresholdBytes is gzipped when it's put
// back into the record, and must decode to the same value.
TEST_F(AIChatSyncBridgeTest, ApplyRestoresOmittedValueThatIsGzipped) {
  std::string answer;
  for (int i = 0; i < 100; ++i) {
    answer += "The answer, at length. ";
  }
  sync_pb::AIChatCompressibleString probe;
  WriteCompressibleString(answer, &probe);
  ASSERT_TRUE(probe.has_gzipped());
  ASSERT_NO_FATAL_FAILURE(SeedCompletion(answer));
  CreateBridge();
  task_environment_.FastForwardBy(base::Minutes(1));

  ASSERT_FALSE(bridge_
                   ->ApplyIncrementalSyncChanges(
                       bridge_->CreateMetadataChangeList(),
                       MakeOmittedCompletionUpdate(answer, base::Time::Now()))
                   .has_value());

  EXPECT_THAT(GetCompletions("conv"), ElementsAre(answer));
  EXPECT_THAT(db_->GetEntryModifiedTime("entry"),
              IsModifiedAt(base::Time::Now()));
}

}  // namespace
}  // namespace ai_chat
