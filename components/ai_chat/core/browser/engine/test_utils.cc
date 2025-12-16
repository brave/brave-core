/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/test_utils.h"

#include <optional>
#include <utility>

#include "base/numerics/clamped_math.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

std::vector<mojom::ConversationTurnPtr> GetHistoryWithModifiedReply() {
  std::vector<mojom::ConversationTurnPtr> history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Which show is 'This is the way' from?", std::nullopt /* prompt */,
      std::nullopt /* selected_text */, std::nullopt /* events */,
      base::Time::Now(), std::nullopt /* edits */,
      std::nullopt /* uploaded_images */, nullptr /* skill */, false,
      std::nullopt /* model_key */, nullptr /* near_verification_status */));

  std::vector<mojom::ConversationEntryEventPtr> events;
  auto search_event = mojom::ConversationEntryEvent::NewSearchStatusEvent(
      mojom::SearchStatusEvent::New());
  events.push_back(search_event.Clone());
  events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("Mandalorian")));

  std::vector<mojom::ConversationEntryEventPtr> modified_events;
  modified_events.push_back(search_event.Clone());
  modified_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("The Mandalorian")));

  auto edit = mojom::ConversationTurn::New(
      "edit-1", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "The Mandalorian.", std::nullopt /* prompt */,
      std::nullopt /* selected_text*/, std::move(modified_events),
      base::Time::Now(), std::nullopt /* edits */,
      std::nullopt /* uploaded_images */, nullptr /* skill */, false,
      "chat-basic", nullptr /* near_verification_status */);
  std::vector<mojom::ConversationTurnPtr> edits;
  edits.push_back(std::move(edit));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Mandalorian.", std::nullopt /* prompt */,
      std::nullopt /* selected_text*/, std::move(events), base::Time::Now(),
      std::move(edits), std::nullopt /* uploaded_images */, nullptr /* skill */,
      false, "chat-basic", nullptr /* near_verification_status */));
  history.push_back(mojom::ConversationTurn::New(
      "turn-3", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Is it related to a broader series?", std::nullopt /* prompt */,
      std::nullopt /* selected_text */, std::nullopt /* events */,
      base::Time::Now(), std::nullopt /* edits */,
      std::nullopt /* uploaded_images */, nullptr /* skill */, false,
      "chat-basic", nullptr /* near_verification_status */));

  return history;
}

void VerifyTextBlock(const base::Location& location,
                     const mojom::ContentBlockPtr& block,
                     std::string_view expected_text) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  ASSERT_EQ(block->which(), mojom::ContentBlock::Tag::kTextContentBlock);
  EXPECT_EQ(block->get_text_content_block()->text, expected_text);
}

void VerifyImageBlock(const base::Location& location,
                      const mojom::ContentBlockPtr& block,
                      const GURL& expected_url) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  ASSERT_EQ(block->which(), mojom::ContentBlock::Tag::kImageContentBlock);
  EXPECT_EQ(block->get_image_content_block()->image_url, expected_url);
}

void VerifyFileBlock(const base::Location& location,
                     const mojom::ContentBlockPtr& block,
                     const GURL& expected_url,
                     std::string_view expected_filename) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  ASSERT_EQ(block->which(), mojom::ContentBlock::Tag::kFileContentBlock);
  EXPECT_EQ(block->get_file_content_block()->file_data, expected_url);
  EXPECT_EQ(block->get_file_content_block()->filename, expected_filename);
}

void VerifyPageTextBlock(const base::Location& location,
                         const mojom::ContentBlockPtr& block,
                         std::string_view expected_text) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  ASSERT_EQ(block->which(), mojom::ContentBlock::Tag::kPageTextContentBlock);
  EXPECT_EQ(block->get_page_text_content_block()->text, expected_text);
}

void VerifyPageExcerptBlock(const base::Location& location,
                            const mojom::ContentBlockPtr& block,
                            std::string_view expected_text) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  ASSERT_EQ(block->which(), mojom::ContentBlock::Tag::kPageExcerptContentBlock);
  EXPECT_EQ(block->get_page_excerpt_content_block()->text, expected_text);
}

base::flat_map<std::string, mojom::MemoryValuePtr> BuildExpectedMemory(
    const base::flat_map<std::string, std::string>& string_values,
    const base::flat_map<std::string, std::vector<std::string>>& list_values) {
  base::flat_map<std::string, mojom::MemoryValuePtr> result;

  for (const auto& [key, value] : string_values) {
    result[key] = mojom::MemoryValue::NewStringValue(value);
  }

  for (const auto& [key, value] : list_values) {
    result[key] = mojom::MemoryValue::NewListValue(value);
  }

  return result;
}

void VerifyMemoryBlock(
    const base::Location& location,
    const mojom::ContentBlockPtr& block,
    const base::flat_map<std::string, mojom::MemoryValuePtr>& expected_memory) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  ASSERT_EQ(block->which(), mojom::ContentBlock::Tag::kMemoryContentBlock);

  const auto& memory_block = block->get_memory_content_block();
  const auto& actual_memory = memory_block->memory;

  // Verify map sizes match
  ASSERT_EQ(actual_memory.size(), expected_memory.size());

  // Verify each key-value pair
  for (const auto& [key, expected_value] : expected_memory) {
    // Verify key exists
    ASSERT_TRUE(actual_memory.contains(key)) << "Key not found: " << key;

    const auto& actual_value = actual_memory.at(key);

    // Verify both values have the same type (union tag)
    ASSERT_EQ(actual_value->which(), expected_value->which());

    // Verify based on type
    switch (expected_value->which()) {
      case mojom::MemoryValue::Tag::kStringValue:
        EXPECT_EQ(actual_value->get_string_value(),
                  expected_value->get_string_value());
        break;
      case mojom::MemoryValue::Tag::kListValue:
        EXPECT_EQ(actual_value->get_list_value(),
                  expected_value->get_list_value());
        break;
    }
  }
}

void VerifyVideoTranscriptBlock(const base::Location& location,
                                const mojom::ContentBlockPtr& block,
                                std::string_view expected_text) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  ASSERT_EQ(block->which(),
            mojom::ContentBlock::Tag::kVideoTranscriptContentBlock);
  EXPECT_EQ(block->get_video_transcript_content_block()->text, expected_text);
}

void VerifyRequestTitleBlock(const base::Location& location,
                             const mojom::ContentBlockPtr& block,
                             std::string_view expected_text) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  ASSERT_EQ(block->which(),
            mojom::ContentBlock::Tag::kRequestTitleContentBlock);
  EXPECT_EQ(block->get_request_title_content_block()->text, expected_text);
}

void VerifyChangeToneBlock(const base::Location& location,
                           const mojom::ContentBlockPtr& block,
                           std::string_view expected_text,
                           std::string_view expected_tone) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  ASSERT_EQ(block->which(), mojom::ContentBlock::Tag::kChangeToneContentBlock);
  EXPECT_EQ(block->get_change_tone_content_block()->text, expected_text);
  EXPECT_EQ(block->get_change_tone_content_block()->tone, expected_tone);
}

void VerifySimpleRequestBlock(const base::Location& location,
                              const mojom::ContentBlockPtr& block,
                              mojom::SimpleRequestType expected_type) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  ASSERT_EQ(block->which(),
            mojom::ContentBlock::Tag::kSimpleRequestContentBlock);
  EXPECT_EQ(block->get_simple_request_content_block()->type, expected_type);
}

}  // namespace ai_chat
