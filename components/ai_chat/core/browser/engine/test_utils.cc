/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/test_utils.h"

#include <optional>
#include <utility>

#include "base/numerics/clamped_math.h"
#include "base/time/time.h"

namespace ai_chat {

std::vector<mojom::ConversationTurnPtr> GetHistoryWithModifiedReply() {
  std::vector<mojom::ConversationTurnPtr> history;
  history.push_back(mojom::ConversationTurn::New(
      "turn-1", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Which show is 'This is the way' from?", std::nullopt /* prompt */,
      std::nullopt /* selected_text */, std::nullopt /* events */,
      base::Time::Now(), std::nullopt /* edits */,
      std::nullopt /* uploaded_images */, nullptr /* skill */, false,
      std::nullopt /* model_key */));

  std::vector<mojom::ConversationEntryEventPtr> events;
  auto search_event = mojom::ConversationEntryEvent::NewSearchStatusEvent(
      mojom::SearchStatusEvent::New());
  events.push_back(search_event.Clone());
  events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("Mandalorian", std::nullopt)));

  std::vector<mojom::ConversationEntryEventPtr> modified_events;
  modified_events.push_back(search_event.Clone());
  modified_events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New("The Mandalorian", std::nullopt)));

  auto edit = mojom::ConversationTurn::New(
      "edit-1", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "The Mandalorian.", std::nullopt /* prompt */,
      std::nullopt /* selected_text*/, std::move(modified_events),
      base::Time::Now(), std::nullopt /* edits */,
      std::nullopt /* uploaded_images */, nullptr /* skill */, false,
      "chat-basic");
  std::vector<mojom::ConversationTurnPtr> edits;
  edits.push_back(std::move(edit));
  history.push_back(mojom::ConversationTurn::New(
      "turn-2", mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      "Mandalorian.", std::nullopt /* prompt */,
      std::nullopt /* selected_text*/, std::move(events), base::Time::Now(),
      std::move(edits), std::nullopt /* uploaded_images */, nullptr /* skill */,
      false, "chat-basic"));
  history.push_back(mojom::ConversationTurn::New(
      "turn-3", mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      "Is it related to a broader series?", std::nullopt /* prompt */,
      std::nullopt /* selected_text */, std::nullopt /* events */,
      base::Time::Now(), std::nullopt /* edits */,
      std::nullopt /* uploaded_images */, nullptr /* skill */, false,
      "chat-basic"));

  return history;
}

}  // namespace ai_chat
