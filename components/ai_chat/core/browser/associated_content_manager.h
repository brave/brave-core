// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_MANAGER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_MANAGER_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "brave/components/ai_chat/core/browser/associated_archive_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

class AssociatedContentManager
    : public ConversationHandler::AssociatedContentDelegate::Observer {
 public:
  explicit AssociatedContentManager(ConversationHandler* conversation);
  ~AssociatedContentManager() override;

  // Sets the content driver for the current conversation (replacing any other
  // content).
  void SetContent(ConversationHandler::AssociatedContentDelegate* delegate);

  // Sets the content from the conversation archive.
  void LoadArchivedContent(
      const mojom::Conversation* metadata,
      const mojom::ConversationArchivePtr& conversation_archive);

  // Replaces |content_id| with some archived content.
  void SetArchiveContent(int content_id,
                         std::string text_content,
                         bool is_video);

  // Adds a content driver to the list of content drivers.
  void AddContent(ConversationHandler::AssociatedContentDelegate* driver,
                  bool notify_updated = true);

  // Removes a content driver from the list of content drivers.
  void RemoveContent(ConversationHandler::AssociatedContentDelegate* driver,
                     bool notify_updated = false);

  void GetContent(ConversationHandler::GetPageContentCallback callback);
  void GetStagedEntriesFromContent(
      ConversationHandler::GetStagedEntriesCallback callback);
  void GetTopSimilarityWithPromptTilContextLimit(
      const std::string& prompt,
      const std::string& text,
      uint32_t context_limit,
      TextEmbedder::TopSimilarityCallback callback);

  std::vector<mojom::AssociatedContentPtr> GetAssociatedContent() const;

  std::string_view GetCachedTextContent();

  bool HasOpenAIChatPermission() const;
  bool HasNonArchiveContent() const;
  bool HasContent() const;

  // Determines if the content for this conversation is a single video.
  bool IsVideo() const;

  // ConversationHandler::AssociatedContentDelegate::Observer
  void OnRequestArchive(
      ConversationHandler::AssociatedContentDelegate* delegate) override;
  void OnTitleChanged(
      ConversationHandler::AssociatedContentDelegate* delegate) override;
  void OnContentChanged(
      ConversationHandler::AssociatedContentDelegate* delegate) override;

  bool should_send() const { return should_send_; }
  void SetShouldSend(bool value);

  std::vector<ConversationHandler::AssociatedContentDelegate*>
  GetContentDriversForTesting() {
    return content_drivers_;
  }

 private:
  void DetachContent();

  bool should_send_ = false;

  raw_ptr<ConversationHandler> conversation_;

  std::vector<ConversationHandler::AssociatedContentDelegate*> content_drivers_;

  // Used for ownership - still stored in the above array.
  std::vector<std::unique_ptr<AssociatedArchiveContent>> archive_content_;

  base::ScopedMultiSourceObservation<
      ConversationHandler::AssociatedContentDelegate,
      ConversationHandler::AssociatedContentDelegate::Observer>
      content_observations_{this};

  std::unique_ptr<base::OneShotEvent> on_page_text_fetch_complete_ = nullptr;
  std::string cached_text_content_;

  base::WeakPtrFactory<AssociatedContentManager> weak_ptr_factory_{this};
};

}  // namespace ai_chat
#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_MANAGER_H_
