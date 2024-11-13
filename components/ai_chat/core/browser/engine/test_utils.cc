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
      mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      mojom::ConversationTurnVisibility::VISIBLE,
      "Which show is 'This is the way' from?", std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, false));

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
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "The Mandalorian.",
      std::nullopt, std::move(modified_events), base::Time::Now(), std::nullopt,
      false);
  std::vector<mojom::ConversationTurnPtr> edits;
  edits.push_back(std::move(edit));
  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, "Mandalorian.", std::nullopt,
      std::move(events), base::Time::Now(), std::move(edits), false));
  history.push_back(mojom::ConversationTurn::New(
      mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      mojom::ConversationTurnVisibility::VISIBLE,
      "Is it related to a broader series?", std::nullopt, std::nullopt,
      base::Time::Now(), std::nullopt, false));

  return history;
}

}  // namespace ai_chat
