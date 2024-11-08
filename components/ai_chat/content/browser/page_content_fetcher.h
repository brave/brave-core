// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PAGE_CONTENT_FETCHER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PAGE_CONTENT_FETCHER_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"

namespace content {
class WebContents;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

class PageContentFetcher : public AIChatTabHelper::PageContentFetcherDelegate {
 public:
  using AIChatTabHelper::PageContentFetcherDelegate::FetchPageContentCallback;

  explicit PageContentFetcher(content::WebContents* web_contents);
  ~PageContentFetcher() override;
  PageContentFetcher(const PageContentFetcher&) = delete;
  PageContentFetcher& operator=(const PageContentFetcher&) = delete;

  void FetchPageContent(std::string_view invalidation_token,
                        FetchPageContentCallback callback) override;

  void GetSearchSummarizerKey(
      mojom::PageContentExtractor::GetSearchSummarizerKeyCallback callback)
      override;

  void GetOpenAIChatButtonNonce(
      mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback)
      override;

  void SetURLLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
    url_loader_factory_ = url_loader_factory;
  }

 private:
  raw_ptr<content::WebContents, DanglingUntriaged> web_contents_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PAGE_CONTENT_FETCHER_H_
