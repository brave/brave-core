// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MEMORY_MANAGER_DELEGATE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MEMORY_MANAGER_DELEGATE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "brave/components/ai_chat/core/common/mojom/customization_settings.mojom.h"

namespace ai_chat {

// What the memory settings handler needs from the rest of the browser: the
// semantic search over the memories (the ones that the user wrote, and the
// learned ones), and the learned memories (Dreaming). The browser layer
// implements it, so that the handler does not depend on the embeddings service
// or on the learned memory code. Without a delegate, or when a feature is off,
// the handler answers that the feature is not available.
class MemoryManagerDelegate {
 public:
  using SearchMemoriesCallback =
      mojom::CustomizationSettingsHandler::SearchMemoriesCallback;
  using GetLearnedMemoriesCallback =
      mojom::CustomizationSettingsHandler::GetLearnedMemoriesCallback;
  using DeleteLearnedMemoryCallback =
      mojom::CustomizationSettingsHandler::DeleteLearnedMemoryCallback;
  using DeleteAllLearnedMemoriesCallback =
      mojom::CustomizationSettingsHandler::DeleteAllLearnedMemoriesCallback;
  using DreamNowCallback =
      mojom::CustomizationSettingsHandler::DreamNowCallback;
  using GetDreamingReviewCallback =
      mojom::CustomizationSettingsHandler::GetDreamingReviewCallback;
  using ApplyDreamingReviewCallback =
      mojom::CustomizationSettingsHandler::ApplyDreamingReviewCallback;

  virtual ~MemoryManagerDelegate() = default;

  // Finds the memories related to `query` by meaning, most related first. A
  // learned memory has its uuid in the result. `callback` gets null while
  // semantic search is unavailable.
  virtual void SearchMemories(const std::string& query,
                              SearchMemoriesCallback callback) = 0;

  // The learned memories, newest first. `available` is false when learned
  // memory is off or the chat database is not ready.
  virtual void GetLearnedMemories(GetLearnedMemoriesCallback callback) = 0;

  // Deletes a learned memory for good. `callback` gets false when it could not.
  virtual void DeleteLearnedMemory(const std::string& uuid,
                                   DeleteLearnedMemoryCallback callback) = 0;

  // Deletes all learned memories. `callback` gets false when it could not.
  virtual void DeleteAllLearnedMemories(
      DeleteAllLearnedMemoriesCallback callback) = 0;

  // Starts a Dreaming run now. `callback` gets the result when the run ends.
  virtual void DreamNow(DreamNowCallback callback) = 0;

  // The changes of the last Dreaming run that wait for the user's review.
  virtual void GetDreamingReview(GetDreamingReviewCallback callback) = 0;

  // Stores the changes in `kept` and discards the others. `callback` gets
  // false when it could not.
  virtual void ApplyDreamingReview(
      std::vector<mojom::LearnedMemoryReviewDecisionPtr> kept,
      ApplyDreamingReviewCallback callback) = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MEMORY_MANAGER_DELEGATE_H_
