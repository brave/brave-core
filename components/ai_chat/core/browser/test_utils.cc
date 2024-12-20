// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/test_utils.h"

#include <string>
#include <utility>

#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

std::string MessageConversationEntryEvents(
    const mojom::ConversationTurnPtr& entry) {
  std::string message = "Entry has the following events:";
  if (!entry->events.has_value()) {
    message = base::StrCat({message, "\nNo events"});
    return message;
  }
  for (const auto& event : entry->events.value()) {
    switch (event->which()) {
      case mojom::ConversationEntryEvent::Tag::kCompletionEvent: {
        message = base::StrCat({message, "\n - completion: ",
                                event->get_completion_event()->completion});
        break;
      }
      case mojom::ConversationEntryEvent::Tag::kSearchQueriesEvent: {
        message = base::StrCat({message, "\n - search event"});
        break;
      }
      case mojom::ConversationEntryEvent::Tag::kConversationTitleEvent: {
        message = base::StrCat({message, "\n - title: ",
                                event->get_conversation_title_event()->title});
        break;
      }
      case mojom::ConversationEntryEvent::Tag::kPageContentRefineEvent: {
        message = base::StrCat({message, "\n - content refine event"});
        break;
      }
      default:
        message = base::StrCat({message, "\n - unknown event"});
    }
  }
  return message;
}

}  // namespace

void ExpectConversationEquals(base::Location location,
                              const mojom::ConversationPtr& a,
                              const mojom::ConversationPtr& b) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  if (!a || !b) {
    EXPECT_EQ(a, b);  // Both should be null or neither
    return;
  }
  EXPECT_EQ(a->uuid, b->uuid);
  EXPECT_EQ(a->title, b->title);
  EXPECT_EQ(a->has_content, b->has_content);

  // associated content
  ExpectAssociatedContentEquals(FROM_HERE, a->associated_content,
                                b->associated_content);
}

void ExpectAssociatedContentEquals(base::Location location,
                                   const mojom::SiteInfoPtr& a,
                                   const mojom::SiteInfoPtr& b) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  if (!a || !b) {
    EXPECT_EQ(a, b);  // Both should be null or neither
    return;
  }
  EXPECT_EQ(a->uuid, b->uuid);
  EXPECT_EQ(a->title, b->title);
  EXPECT_EQ(a->url, b->url);
  EXPECT_EQ(a->content_type, b->content_type);
  EXPECT_EQ(a->content_used_percentage, b->content_used_percentage);
  EXPECT_EQ(a->is_content_refined, b->is_content_refined);
  EXPECT_EQ(a->is_content_association_possible,
            b->is_content_association_possible);
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
  if (!a || !b) {
    EXPECT_EQ(a, b);  // Both should be null or neither
    return;
  }

  if (compare_uuid) {
    EXPECT_EQ(a->uuid.value_or("default"), b->uuid.value_or("default"));
  }

  EXPECT_EQ(a->action_type, b->action_type);
  EXPECT_EQ(a->character_type, b->character_type);
  EXPECT_EQ(a->selected_text, b->selected_text);
  EXPECT_EQ(a->text, b->text);
  EXPECT_EQ(a->prompt, b->prompt);

  // compare events
  EXPECT_EQ(a->events.has_value(), b->events.has_value());
  if (a->events.has_value()) {
    EXPECT_EQ(a->events->size(), b->events->size())
        << "\nEvents for a. " << MessageConversationEntryEvents(a)
        << "\nEvents for b. " << MessageConversationEntryEvents(b);
    for (auto i = 0u; i < a->events->size(); i++) {
      SCOPED_TRACE(testing::Message() << "Comparing events at index " << i);
      auto& a_event = a->events->at(i);
      auto& b_event = b->events->at(i);
      EXPECT_EQ(a_event->which(), b_event->which());
      switch (a_event->which()) {
        case mojom::ConversationEntryEvent::Tag::kCompletionEvent: {
          EXPECT_EQ(a_event->get_completion_event()->completion,
                    b_event->get_completion_event()->completion);
          break;
        }
        case mojom::ConversationEntryEvent::Tag::kSearchQueriesEvent: {
          EXPECT_EQ(a_event->get_search_queries_event()->search_queries,
                    b_event->get_search_queries_event()->search_queries);
          break;
        }
        default:
          NOTREACHED()
              << "Unexpected event type for comparison. Only know about "
                 "event types which are not discarded.";
      }
    }
  }

  // compare edits
  EXPECT_EQ(a->edits.has_value(), b->edits.has_value());
  if (a->edits.has_value()) {
    EXPECT_EQ(a->edits->size(), b->edits->size());
    for (auto i = 0u; i < a->edits->size(); i++) {
      SCOPED_TRACE(testing::Message() << "Comparing edits at index " << i);
      auto& a_edit = a->edits->at(i);
      auto& b_edit = b->edits->at(i);
      ExpectConversationEntryEquals(FROM_HERE, a_edit, b_edit, compare_uuid);
    }
  }
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
    int32_t future_hours) {
  std::vector<mojom::ConversationTurnPtr> history;
  base::Time now = base::Time::Now();
  for (size_t i = 0; i < num_query_pairs; i++) {
    // query
    history.push_back(mojom::ConversationTurn::New(
        base::Uuid::GenerateRandomV4().AsLowercaseString(),
        mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
        base::StrCat({"query", base::NumberToString(i)}),
        std::nullopt /* prompt */, std::nullopt, std::nullopt,
        now + base::Seconds(i * 60) + base::Hours(future_hours), std::nullopt,
        false));
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
        std::nullopt, false));
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

}  // namespace ai_chat
