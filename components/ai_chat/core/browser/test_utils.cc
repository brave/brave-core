// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/test_utils.h"

#include <string>
#include <utility>

#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

base::Time TruncateToSeconds(base::Time time) {
  base::Time::Exploded exploded;
  time.UTCExplode(&exploded);
  exploded.millisecond = 0;
  EXPECT_TRUE(base::Time::FromUTCExploded(exploded, &time));
  return time;
}

mojom::ConversationTurnPtr CloneForCompare(
    const mojom::ConversationTurnPtr& turn,
    bool compare_uuid) {
  auto cloned_turn = turn->Clone();
  if (!compare_uuid) {
    cloned_turn->uuid = "";
  }
  // we don't need to compare the times to the micro second
  cloned_turn->created_time = TruncateToSeconds(cloned_turn->created_time);

  // Do the same for edits
  if (cloned_turn->edits) {
    std::vector<mojom::ConversationTurnPtr> transformed_edits;
    for (const auto& edit : cloned_turn->edits.value()) {
      transformed_edits.push_back(CloneForCompare(edit, compare_uuid));
    }
    cloned_turn->edits = std::move(transformed_edits);
  }
  return cloned_turn;
}

}  // namespace

void ExpectConversationEquals(base::Location location,
                              const mojom::ConversationPtr& a_raw,
                              const mojom::ConversationPtr& b_raw,
                              bool compare_non_persisted_fields) {
  auto a = a_raw->Clone();
  auto b = b_raw->Clone();
  if (!compare_non_persisted_fields) {
    // has_content is not persisted
    a->has_content = false;
    b->has_content = false;
    // Date is not persisted
    a->updated_time = base::Time::Now();
    b->updated_time = base::Time::Now();
    // content_id is not persisted
    for (auto& content : a->associated_content) {
      content->content_id = 0;
    }
    for (auto& content : b->associated_content) {
      content->content_id = 0;
    }
  }
  SCOPED_TRACE(testing::Message() << location.ToString());
  EXPECT_MOJOM_EQ(*a, *b);
}

void ExpectAssociatedContentEquals(
    base::Location location,
    const std::vector<mojom::AssociatedContentPtr>& a,
    const std::vector<mojom::AssociatedContentPtr>& b,
    bool compare_non_persisted_fields) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  EXPECT_EQ(a.size(), b.size());

  for (size_t i = 0; i < a.size(); ++i) {
    SCOPED_TRACE(testing::Message()
                 << "Comparing associated content at index " << i);
    const auto& a_content_raw = a[i];
    const auto& b_content_raw = b[i];

    if (!a_content_raw || !b_content_raw) {
      EXPECT_EQ(a_content_raw,
                b_content_raw);  // Both should be null or neither
      return;
    }

    auto a_content = a_content_raw->Clone();
    auto b_content = b_content_raw->Clone();
    if (!compare_non_persisted_fields) {
      a_content->content_id = 0;
      b_content->content_id = 0;
    }

    EXPECT_MOJOM_EQ(*a_content, *b_content);
  }
}

void ExpectConversationHistoryEquals(
    base::Location location,
    const std::vector<mojom::ConversationTurnPtr>& a,
    const std::vector<mojom::ConversationTurnPtr>& b,
    bool compare_uuid) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  EXPECT_EQ(a.size(), b.size());
  for (auto i = 0u; i < a.size(); i++) {
    SCOPED_TRACE(testing::Message() << "Comparing entries at index " << i);
    ExpectConversationEntryEquals(FROM_HERE, a.at(i), b.at(i), compare_uuid);
  }
}

void ExpectConversationEntryEquals(base::Location location,
                                   const mojom::ConversationTurnPtr& a,
                                   const mojom::ConversationTurnPtr& b,
                                   bool compare_uuid) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  auto a_compare = CloneForCompare(a, compare_uuid);
  auto b_compare = CloneForCompare(b, compare_uuid);

  EXPECT_MOJOM_EQ(*a_compare, *b_compare);
}

mojom::Conversation* GetConversation(
    base::Location location,
    const std::vector<mojom::ConversationPtr>& conversations,
    std::string uuid) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  auto it = std::find_if(conversations.begin(), conversations.end(),
                         [&uuid](const mojom::ConversationPtr& conversation) {
                           return conversation->uuid == uuid;
                         });
  EXPECT_NE(it, conversations.end());
  return it->get();
}

std::vector<mojom::ConversationTurnPtr> CreateSampleChatHistory(
    size_t num_query_pairs,
    int32_t future_hours,
    size_t num_uploaded_files_per_query) {
  std::vector<mojom::ConversationTurnPtr> history;
  base::Time now = base::Time::Now();
  for (size_t i = 0; i < num_query_pairs; i++) {
    // query
    std::optional<std::vector<mojom::UploadedFilePtr>> uploaded_files;
    if (num_uploaded_files_per_query) {
      uploaded_files = CreateSampleUploadedFiles(
          num_uploaded_files_per_query, mojom::UploadedFileType::kImage);
    }
    history.push_back(mojom::ConversationTurn::New(
        base::Uuid::GenerateRandomV4().AsLowercaseString(),
        mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
        base::StrCat({"query", base::NumberToString(i)}),
        std::nullopt /* prompt */, std::nullopt, std::nullopt,
        now + base::Seconds(i * 60) + base::Hours(future_hours), std::nullopt,
        std::move(uploaded_files), nullptr /* skill */, false,
        std::nullopt /* model_key */, nullptr /* near_verification_status */));
    // response
    std::vector<mojom::ConversationEntryEventPtr> events;
    events.emplace_back(mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(base::StrCat(
            {"This is a generated response ", base::NumberToString(i)}))));
    events.emplace_back(mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(base::StrCat(
            {"and this is more response", base::NumberToString(i)}))));
    events.emplace_back(mojom::ConversationEntryEvent::NewSearchQueriesEvent(
        mojom::SearchQueriesEvent::New(std::vector<std::string>{
            base::StrCat({"Something to search for", base::NumberToString(i)}),
            base::StrCat({"Another search query", base::NumberToString(i)})})));
    history.push_back(mojom::ConversationTurn::New(
        base::Uuid::GenerateRandomV4().AsLowercaseString(),
        mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE, "",
        std::nullopt /* prompt */, std::nullopt, std::move(events),
        now + base::Seconds((i * 60) + 30) + base::Hours(future_hours),
        std::nullopt, std::nullopt, nullptr /* skill */, false, "chat-basic",
        nullptr));
  }
  return history;
}

std::vector<mojom::ConversationTurnPtr> CloneHistory(
    std::vector<mojom::ConversationTurnPtr>& history) {
  std::vector<mojom::ConversationTurnPtr> cloned_history;
  for (const auto& turn : history) {
    cloned_history.push_back(turn->Clone());
  }
  return cloned_history;
}

void WaitForAssociatedContentFetch(AssociatedContentManager* manager) {
  base::RunLoop run_loop;
  manager->GetContent(run_loop.QuitClosure());
  run_loop.Run();
}

}  // namespace ai_chat
