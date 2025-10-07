// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/test/mock_associated_content.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest-death-test.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::NiceMock;

namespace ai_chat {

namespace {

class MockAIChatCredentialManager : public AIChatCredentialManager {
 public:
  using AIChatCredentialManager::AIChatCredentialManager;
  void GetPremiumStatus(
      mojom::Service::GetPremiumStatusCallback callback) override {
    std::move(callback).Run(mojom::PremiumStatus::Inactive,
                            mojom::PremiumInfo::New());
  }
};

}  // namespace

class AssociatedContentManagerUnitTest : public testing::Test {
 public:
  AssociatedContentManagerUnitTest() : feedback_api_(nullptr, "") {}

  void SetUp() override {
    ASSERT_TRUE(temp_directory_.CreateUniqueTempDir());
    prefs::RegisterProfilePrefs(prefs_.registry());
    prefs::RegisterLocalStatePrefs(local_state_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());

    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);

    model_service_ = std::make_unique<ModelService>(&prefs_);

    ai_chat_service_ = std::make_unique<AIChatService>(
        model_service_.get(), nullptr /* tab_tracker_service */,
        std::make_unique<MockAIChatCredentialManager>(base::NullCallback(),
                                                      &local_state_),
        &prefs_, nullptr, os_crypt_.get(), shared_url_loader_factory_, "",
        temp_directory_.GetPath());

    conversation_ = mojom::Conversation::New(
        "uuid", "title", base::Time::Now(), false, std::nullopt, 0, 0, false,
        std::vector<mojom::AssociatedContentPtr>());

    conversation_handler_ = std::make_unique<ConversationHandler>(
        conversation_.get(), ai_chat_service_.get(), model_service_.get(),
        ai_chat_service_->GetCredentialManagerForTesting(), &feedback_api_,
        &prefs_, shared_url_loader_factory_,
        std::vector<std::unique_ptr<ToolProvider>>());

    conversation_handler_->SetEngineForTesting(
        std::make_unique<NiceMock<MockEngineConsumer>>());
  }

  void TearDown() override { ai_chat_service_.reset(); }

 protected:
  base::test::TaskEnvironment task_environment_;
  AIChatFeedbackAPI feedback_api_;
  std::unique_ptr<AIChatService> ai_chat_service_;
  std::unique_ptr<ModelService> model_service_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  mojom::ConversationPtr conversation_;

  std::unique_ptr<ConversationHandler> conversation_handler_;

 private:
  base::ScopedTempDir temp_directory_;
};

TEST_F(AssociatedContentManagerUnitTest,
       AssociateUnsentContentWithTurn_BasicAssociation) {
  NiceMock<MockAssociatedContent> content;
  conversation_handler_->associated_content_manager()->AddContent(&content);

  auto turn = mojom::ConversationTurn::New(
      "test-turn-uuid", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Test human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);

  // Initially, GetAssociatedContent should not have conversation_turn_uuid set
  auto initial_content = conversation_handler_->associated_content_manager()
                             ->GetAssociatedContent();
  ASSERT_EQ(1u, initial_content.size());
  EXPECT_FALSE(initial_content[0]->conversation_turn_uuid.has_value());

  // Associate content with turn
  conversation_handler_->associated_content_manager()
      ->AssociateUnsentContentWithTurn(turn);

  conversation_handler_->associated_content_manager()->AddContent(&content);

  // After association, GetAssociatedContent should have conversation_turn_uuid
  // set
  auto associated_content = conversation_handler_->associated_content_manager()
                                ->GetAssociatedContent();
  ASSERT_EQ(1u, associated_content.size());
  EXPECT_TRUE(associated_content[0]->conversation_turn_uuid.has_value());
  EXPECT_EQ("test-turn-uuid",
            associated_content[0]->conversation_turn_uuid.value());
  EXPECT_EQ(content.uuid(), associated_content[0]->uuid);

  // GetCachedContentsMap should work without crashing and include the turn UUID
  // as key
  auto contents_map = conversation_handler_->associated_content_manager()
                          ->GetCachedContentsMap();
  EXPECT_TRUE(contents_map.contains("test-turn-uuid"));
}

TEST_F(AssociatedContentManagerUnitTest,
       AssociateUnsentContentWithTurn_MultipleContent) {
  NiceMock<MockAssociatedContent> first_content;
  conversation_handler_->associated_content_manager()->AddContent(
      &first_content);

  // Add a second content delegate
  NiceMock<MockAssociatedContent> second_content;
  conversation_handler_->associated_content_manager()->AddContent(
      &second_content);

  auto turn = mojom::ConversationTurn::New(
      "test-turn-uuid", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Test human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);

  // Associate content with turn
  conversation_handler_->associated_content_manager()
      ->AssociateUnsentContentWithTurn(turn);

  // Both content items should be associated with the turn
  auto associated_content = conversation_handler_->associated_content_manager()
                                ->GetAssociatedContent();
  ASSERT_EQ(2u, associated_content.size());

  // Both should have the same conversation_turn_uuid
  EXPECT_TRUE(associated_content[0]->conversation_turn_uuid.has_value());
  EXPECT_TRUE(associated_content[1]->conversation_turn_uuid.has_value());
  EXPECT_EQ("test-turn-uuid",
            associated_content[0]->conversation_turn_uuid.value());
  EXPECT_EQ("test-turn-uuid",
            associated_content[1]->conversation_turn_uuid.value());

  // UUIDs should be match
  EXPECT_EQ(first_content.uuid(), associated_content[0]->uuid);
  EXPECT_EQ(second_content.uuid(), associated_content[1]->uuid);

  // GetCachedContentsMap should work and have both content items under the same
  // turn UUID
  auto contents_map = conversation_handler_->associated_content_manager()
                          ->GetCachedContentsMap();
  EXPECT_TRUE(contents_map.contains("test-turn-uuid"));
  EXPECT_EQ(2u, contents_map.at("test-turn-uuid").size());
}

TEST_F(AssociatedContentManagerUnitTest,
       AssociateUnsentContentWithTurn_MultipleContent_MultipleTurns) {
  NiceMock<MockAssociatedContent> first_content;
  first_content.SetTextContent("Page 1 content");

  conversation_handler_->associated_content_manager()->AddContent(
      &first_content);

  auto turn1 = mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Test human message 1", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);

  auto turn2 = mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Test human message 2", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);

  // Associate the first content with turn 1
  conversation_handler_->associated_content_manager()
      ->AssociateUnsentContentWithTurn(turn1);

  auto associated_content = conversation_handler_->associated_content_manager()
                                ->GetAssociatedContent();
  ASSERT_EQ(1u, associated_content.size());
  EXPECT_EQ("turn-1", associated_content[0]->conversation_turn_uuid);

  // Add a second content delegate
  NiceMock<MockAssociatedContent> second_content;
  second_content.SetTextContent("Page 2 content");

  conversation_handler_->associated_content_manager()->AddContent(
      &second_content);
  conversation_handler_->associated_content_manager()
      ->AssociateUnsentContentWithTurn(turn2);

  // Both content items should be associated with the turn
  associated_content = conversation_handler_->associated_content_manager()
                           ->GetAssociatedContent();
  ASSERT_EQ(2u, associated_content.size());

  // First content should be associated with turn 1.
  EXPECT_EQ("turn-1", associated_content[0]->conversation_turn_uuid);
  // Second content should be associated with turn 2.
  EXPECT_EQ("turn-2", associated_content[1]->conversation_turn_uuid);

  // GetCachedContentsMap should work and have both content items under their
  // associated turns.
  auto contents_map = conversation_handler_->associated_content_manager()
                          ->GetCachedContentsMap();
  EXPECT_TRUE(base::Contains(contents_map, "turn-1"));
  ASSERT_EQ(1u, contents_map.at("turn-1").size());
  EXPECT_EQ("Page 1 content", contents_map.at("turn-1")[0].get().content);

  EXPECT_TRUE(base::Contains(contents_map, "turn-2"));
  ASSERT_EQ(1u, contents_map.at("turn-2").size());
  EXPECT_EQ("Page 2 content", contents_map.at("turn-2")[0].get().content);
}

TEST_F(AssociatedContentManagerUnitTest,
       AssociateUnsentContentWithTurn_AlreadyAssociated) {
  NiceMock<MockAssociatedContent> content;
  conversation_handler_->associated_content_manager()->AddContent(&content);

  auto turn1 = mojom::ConversationTurn::New(
      "test-turn-uuid-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "First human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);

  auto turn2 = mojom::ConversationTurn::New(
      "test-turn-uuid-2", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Second human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);

  // Associate content with first turn
  conversation_handler_->associated_content_manager()
      ->AssociateUnsentContentWithTurn(turn1);

  // Verify association
  auto content_after_first = conversation_handler_->associated_content_manager()
                                 ->GetAssociatedContent();
  ASSERT_EQ(1u, content_after_first.size());
  EXPECT_EQ("test-turn-uuid-1",
            content_after_first[0]->conversation_turn_uuid.value());

  // Try to associate the same content with second turn - should be skipped
  conversation_handler_->associated_content_manager()
      ->AssociateUnsentContentWithTurn(turn2);

  // Content should still be associated with first turn
  auto content_after_second =
      conversation_handler_->associated_content_manager()
          ->GetAssociatedContent();
  ASSERT_EQ(1u, content_after_second.size());
  EXPECT_EQ("test-turn-uuid-1",
            content_after_second[0]->conversation_turn_uuid.value());
}

TEST_F(AssociatedContentManagerUnitTest,
       AssociateUnsentContentWithTurn_RequiresUuid) {
  // Create turn without UUID - should crash
  auto turn_without_uuid = mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Test human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);

  EXPECT_DEATH_IF_SUPPORTED(
      conversation_handler_->associated_content_manager()
          ->AssociateUnsentContentWithTurn(turn_without_uuid),
      "");
}

TEST_F(AssociatedContentManagerUnitTest, GetCachedContentsMap_Empty) {
  // GetCachedContentsMap should return an empty map when there's no content
  auto contents_map = conversation_handler_->associated_content_manager()
                          ->GetCachedContentsMap();
  EXPECT_TRUE(contents_map.empty());
}

TEST_F(AssociatedContentManagerUnitTest,
       GetCachedContentsMap_UnassociatedContent) {
  // Add content but don't associate it with any turn
  NiceMock<MockAssociatedContent> content;
  content.SetTextContent("Unassociated content");
  conversation_handler_->associated_content_manager()->AddContent(&content);

  auto associated_content = conversation_handler_->associated_content_manager()
                                ->GetAssociatedContent();
  ASSERT_EQ(1u, associated_content.size());
  EXPECT_FALSE(associated_content[0]->conversation_turn_uuid.has_value());

#if DCHECK_IS_ON()
  // This will only crash if DCHECK is on.
  EXPECT_DEATH_IF_SUPPORTED(conversation_handler_->associated_content_manager()
                                ->GetCachedContentsMap(),
                            "");
#else
  // If DCHECK is off, the map should be empty.
  auto contents_map = conversation_handler_->associated_content_manager()
                          ->GetCachedContentsMap();
  EXPECT_TRUE(contents_map.empty());
#endif
}

TEST_F(AssociatedContentManagerUnitTest, GetCachedContentsMap_MultipleContent) {
  NiceMock<MockAssociatedContent> content1;
  content1.SetTextContent("Content 1");
  conversation_handler_->associated_content_manager()->AddContent(&content1);

  NiceMock<MockAssociatedContent> content2;
  content2.SetTextContent("Content 2");
  conversation_handler_->associated_content_manager()->AddContent(&content2);

  auto turn = mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Test human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);

  conversation_handler_->associated_content_manager()
      ->AssociateUnsentContentWithTurn(turn);

  auto contents_map = conversation_handler_->associated_content_manager()
                          ->GetCachedContentsMap();
  ASSERT_TRUE(contents_map.contains("turn-1"));
  ASSERT_EQ(2u, contents_map.at("turn-1").size());
  EXPECT_EQ("Content 1", contents_map.at("turn-1")[0].get().content);
  EXPECT_EQ("Content 2", contents_map.at("turn-1")[1].get().content);
}

TEST_F(AssociatedContentManagerUnitTest,
       GetCachedContentsMap_MultipleContent_MultipleTurns) {
  NiceMock<MockAssociatedContent> content1;
  content1.SetTextContent("Content 1");
  conversation_handler_->associated_content_manager()->AddContent(&content1);

  NiceMock<MockAssociatedContent> content2;
  content2.SetTextContent("Content 2");
  conversation_handler_->associated_content_manager()->AddContent(&content2);

  // Associate content 1 & 2 with turn 1
  auto turn1 = mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Test human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);

  conversation_handler_->associated_content_manager()
      ->AssociateUnsentContentWithTurn(turn1);

  NiceMock<MockAssociatedContent> content3;
  content3.SetTextContent("Content 3");
  conversation_handler_->associated_content_manager()->AddContent(&content3);

  auto turn2 = mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Test human message", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);

  // Associate content 3 with turn 2
  conversation_handler_->associated_content_manager()
      ->AssociateUnsentContentWithTurn(turn2);

  auto contents_map = conversation_handler_->associated_content_manager()
                          ->GetCachedContentsMap();

  ASSERT_TRUE(contents_map.contains("turn-1"));
  ASSERT_EQ(2u, contents_map.at("turn-1").size());
  EXPECT_EQ("Content 1", contents_map.at("turn-1")[0].get().content);
  EXPECT_EQ("Content 2", contents_map.at("turn-1")[1].get().content);

  ASSERT_TRUE(contents_map.contains("turn-2"));
  ASSERT_EQ(1u, contents_map.at("turn-2").size());
  EXPECT_EQ("Content 3", contents_map.at("turn-2")[0].get().content);
}

TEST_F(AssociatedContentManagerUnitTest,
       GetCachedContentsMap_WithRemovedContent) {
  // Test that removed content doesn't appear in the cached contents map
  NiceMock<MockAssociatedContent> content_to_keep;
  content_to_keep.SetTextContent("Keep this content");
  NiceMock<MockAssociatedContent> content_to_remove;
  content_to_remove.SetTextContent("Remove this content");

  conversation_handler_->associated_content_manager()->AddContent(
      &content_to_keep);
  conversation_handler_->associated_content_manager()->AddContent(
      &content_to_remove);

  auto turn = mojom::ConversationTurn::New(
      "removal-turn", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Removal test", std::nullopt, std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, std::nullopt, nullptr /* smart_mode */,
      false, std::nullopt);

  // Associate both content items with the turn
  conversation_handler_->associated_content_manager()
      ->AssociateUnsentContentWithTurn(turn);

  // Verify both are in the map initially
  auto contents_map_before = conversation_handler_->associated_content_manager()
                                 ->GetCachedContentsMap();
  EXPECT_TRUE(contents_map_before.contains("removal-turn"));
  EXPECT_EQ(2u, contents_map_before.at("removal-turn").size());

  // Remove one content item
  conversation_handler_->associated_content_manager()->RemoveContent(
      &content_to_remove);

  // Verify only the kept content remains in the map
  auto contents_map_after = conversation_handler_->associated_content_manager()
                                ->GetCachedContentsMap();
  EXPECT_TRUE(contents_map_after.contains("removal-turn"));
  ASSERT_EQ(1u, contents_map_after.at("removal-turn").size());
  EXPECT_EQ("Keep this content",
            contents_map_after.at("removal-turn")[0].get().content);
}

TEST_F(AssociatedContentManagerUnitTest,
       AddContent_TriggersUpdateAndNotifiesConversation) {
  // Test that removed content doesn't appear in the cached contents map
  NiceMock<MockAssociatedContent> associated_content;
  associated_content.SetTextContent("Some video transcript");
  associated_content.SetIsVideo(true);

  // Should have empty cached page content.
  EXPECT_TRUE(associated_content.cached_page_content().content.empty());
  EXPECT_FALSE(associated_content.cached_page_content().is_video);

  // Conversation metadata should have no associated content.
  EXPECT_TRUE(conversation_->associated_content.empty());

  conversation_handler_->associated_content_manager()->AddContent(
      &associated_content);

  // GetContent should have been called when adding the content to the manager.
  EXPECT_EQ("Some video transcript",
            associated_content.cached_page_content().content);
  EXPECT_TRUE(associated_content.cached_page_content().is_video);

  // Conversation metadata should have been updated now the AssociatedContent
  // knows its a video.
  ASSERT_EQ(1u, conversation_->associated_content.size());
  EXPECT_EQ(conversation_->associated_content[0]->content_type,
            mojom::ContentType::VideoTranscript);
}

}  // namespace ai_chat
