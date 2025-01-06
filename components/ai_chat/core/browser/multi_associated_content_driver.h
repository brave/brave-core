// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MULTI_ASSOCIATED_CONTENT_DRIVER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MULTI_ASSOCIATED_CONTENT_DRIVER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/scoped_multi_source_observation.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"

namespace ai_chat {

class MultiAssociatedContentDriver
    : public ConversationHandler::AssociatedContentDelegate,
      public AssociatedContentDriver::Observer {
 public:
  explicit MultiAssociatedContentDriver(
      std::vector<AssociatedContentDriver*> content);

  MultiAssociatedContentDriver(const MultiAssociatedContentDriver&) = delete;
  MultiAssociatedContentDriver& operator=(const MultiAssociatedContentDriver&) =
      delete;
  ~MultiAssociatedContentDriver() override;

  void AddContent(AssociatedContentDriver* content);
  void RemoveContent(AssociatedContentDriver* content);

  int GetContentCount() const;

  // AssociatedContentDriver::Observer
  void OnAssociatedContentDestroyed(AssociatedContentDriver* content) override;

  // ConversationHandler::AssociatedContentDelegate
  void AddRelatedConversation(ConversationHandler* conversation) override;
  void OnRelatedConversationDisassociated(
      ConversationHandler* conversation) override;
  int GetContentId() const override;
  GURL GetURL() const override;
  std::u16string GetTitle() const override;
  std::vector<mojom::SiteInfoDetailPtr> GetSiteInfoDetail() const override;
  void GetContent(
      ConversationHandler::GetPageContentCallback callback) override;
  std::string_view GetCachedTextContent() override;
  bool GetCachedIsVideo() override;
  void GetStagedEntriesFromContent(
      ConversationHandler::GetStagedEntriesCallback callback) override;
  bool HasOpenAIChatPermission() const override;

  base::WeakPtr<MultiAssociatedContentDriver> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  std::set<raw_ptr<ConversationHandler>> associated_conversations_;
  std::vector<AssociatedContentDriver*> content_;
  base::ScopedMultiSourceObservation<AssociatedContentDriver,
                                     AssociatedContentDriver::Observer>
      content_observations_{this};

  std::unique_ptr<base::OneShotEvent> on_page_text_fetch_complete_ = nullptr;
  std::string cached_text_content_;
  std::string content_invalidation_token_;
  bool is_video_ = false;

  base::WeakPtrFactory<MultiAssociatedContentDriver> weak_ptr_factory_{this};
};
}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MULTI_ASSOCIATED_CONTENT_DRIVER_H_
