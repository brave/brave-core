/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_DRIVER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_DRIVER_H_

#include <memory>
#include <set>
#include <string>
#include <string_view>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/one_shot_event.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"

class PrefService;

FORWARD_DECLARE_TEST(AIChatUIBrowserTest, PrintPreviewFallback);
namespace ai_chat {
class AIChatMetrics;

// Contains a platform-independent relationship between multiple conversations
// and a single piece of content. Must be subclassed to
// provide the platform-specific retrieval of the content details, such as
// extracting the content of a web page.
class AssociatedContentDriver
    : public ConversationHandler::AssociatedContentDelegate {
 public:
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    virtual void OnAssociatedContentNavigated(int new_navigation_id) {}
  };

  explicit AssociatedContentDriver(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~AssociatedContentDriver() override;

  AssociatedContentDriver(const AssociatedContentDriver&) = delete;
  AssociatedContentDriver& operator=(const AssociatedContentDriver&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // ConversationHandler::AssociatedContentDelegate
  void AddRelatedConversation(ConversationHandler* conversation) override;
  void OnRelatedConversationDestroyed(
      ConversationHandler* conversation) override;
  int GetContentId() const override;
  GURL GetURL() const override;
  std::u16string GetTitle() const override;
  void GetContent(
      ConversationHandler::GetPageContentCallback callback) override;
  std::string_view GetCachedTextContent() override;
  bool GetCachedIsVideo() override;
  void GetStagedEntriesFromContent(
      ConversationHandler::GetStagedEntriesCallback callback) override;

  base::WeakPtr<AssociatedContentDriver> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 protected:
  virtual GURL GetPageURL() const = 0;
  virtual std::u16string GetPageTitle() const = 0;
  // Get summarizer-key meta tag content from Brave Search SERP if exists.
  virtual void GetSearchSummarizerKey(
      mojom::PageContentExtractor::GetSearchSummarizerKeyCallback callback) = 0;

  // Implementer should fetch content from the "page" associated with this
  // conversation.
  // |invalidation_token| is an optional parameter received in a prior callback
  // response of this function against the same page. See GetPageContentCallback
  // for explanation.
  virtual void GetPageContent(
      ConversationHandler::GetPageContentCallback callback,
      std::string_view invalidation_token) = 0;

  virtual void OnFaviconImageDataChanged();

  // Implementer should call this when the content is updated in a way that
  // will not be detected by the on-demand techniques used by GetPageContent.
  // For example for sites where GetPageContent does not read the live DOM but
  // reads static JS from HTML that doesn't change for same-page navigation and
  // we need to intercept new JS data from subresource loads.
  void OnPageContentUpdated(std::string content,
                            bool is_video,
                            std::string invalidation_token);

  // To be called when a page navigation is detected and a new conversation
  // is expected.
  void OnNewPage(int64_t navigation_id) override;

 private:
  friend class ::AIChatUIBrowserTest;
  FRIEND_TEST_ALL_PREFIXES(::AIChatUIBrowserTest, PrintPreviewFallback);
  FRIEND_TEST_ALL_PREFIXES(AssociatedContentDriverUnitTest,
                           ParseSearchQuerySummaryResponse);

  void OnGeneratePageContentComplete(int64_t navigation_id,
                                     std::string contents_text,
                                     bool is_video,
                                     std::string invalidation_token);
  void OnExistingGeneratePageContentComplete(
      ConversationHandler::GetPageContentCallback callback,
      int64_t navigation_id);

  void OnSearchSummarizerKeyFetched(
      ConversationHandler::GetStagedEntriesCallback callback,
      int64_t navigation_id,
      const std::optional<std::string>& key);
  void OnSearchQuerySummaryFetched(
      ConversationHandler::GetStagedEntriesCallback callback,
      int64_t navigation_id,
      api_request_helper::APIRequestResult result);
  static std::optional<SearchQuerySummary> ParseSearchQuerySummaryResponse(
      const base::Value& value);

  raw_ptr<PrefService> pref_service_;
  raw_ptr<AIChatMetrics> ai_chat_metrics_;
  std::unique_ptr<AIChatCredentialManager> credential_manager_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // Used for fetching search query summary.
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  base::ObserverList<Observer> observers_;
  std::unique_ptr<base::OneShotEvent> on_page_text_fetch_complete_;
  bool is_page_text_fetch_in_progress_ = false;
  std::string cached_text_content_;
  std::string content_invalidation_token_;
  bool is_video_ = false;

  // Handlers that are interested in this content for the current navigation.
  std::set<raw_ptr<ConversationHandler>> associated_conversations_;

  // Store the unique ID for each "page" so that
  // we can ignore API async responses against any navigated-away-from
  // documents.
  int64_t current_navigation_id_;

  base::WeakPtrFactory<AssociatedContentDriver> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_DRIVER_H_
