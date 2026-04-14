// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_PAGE_CONTENT_FETCHER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_PAGE_CONTENT_FETCHER_H_

#include <memory>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/content/browser/associated_web_contents_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "components/optimization_guide/content/browser/page_content_proto_provider.h"

namespace content {
class WebContents;
}

namespace ai_chat {
class PageContentFetcher;

// Implements PageContentFetcherDelegate using
// optimization_guide::GetAIPageContent to provide structured page content
// suitable for AI Chat conversations. For utility methods
// (GetSearchSummarizerKey, GetOpenAIChatButtonNonce), delegates to a
// PageContentFetcher which uses the renderer IPC path.
class AIPageContentFetcher
    : public AssociatedWebContentsContent::PageContentFetcherDelegate {
 public:
  using FetchPageContentCallback =
      AssociatedContentDriver::FetchPageContentCallback;

  explicit AIPageContentFetcher(content::WebContents* web_contents);
  ~AIPageContentFetcher() override;

  AIPageContentFetcher(const AIPageContentFetcher&) = delete;
  AIPageContentFetcher& operator=(const AIPageContentFetcher&) = delete;

  // AssociatedWebContentsContent::PageContentFetcherDelegate:
  void FetchPageContent(std::string_view invalidation_token,
                        FetchPageContentCallback callback) override;
  void GetSearchSummarizerKey(
      mojom::PageContentExtractor::GetSearchSummarizerKeyCallback callback)
      override;
  void GetOpenAIChatButtonNonce(
      mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback)
      override;

 protected:
  // Calls optimization_guide::GetAIPageContent. Virtual for testing.
  virtual void CallGetAIPageContent(
      optimization_guide::OnAIPageContentDone callback);

 private:
  void OnAIPageContentReceived(
      FetchPageContentCallback callback,
      optimization_guide::AIPageContentResultOrError result);

  raw_ptr<content::WebContents> web_contents_;  // not owned
  std::unique_ptr<PageContentFetcher> page_content_fetcher_;
  base::WeakPtrFactory<AIPageContentFetcher> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_PAGE_CONTENT_FETCHER_H_
