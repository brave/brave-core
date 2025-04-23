// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_AI_CHAT_PAGE_CONTENT_FETCHER_H_
#define BRAVE_IOS_BROWSER_API_AI_CHAT_PAGE_CONTENT_FETCHER_H_

#include <string>
#include <string_view>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/ios/browser/api/ai_chat/ai_chat_tab_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace web {
class WebState;
}  // namespace web

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

class PageContentFetcher : public AIChatTabHelper::PageContentFetcherDelegate {
 public:
  using AIChatTabHelper::PageContentFetcherDelegate::FetchPageContentCallback;

  explicit PageContentFetcher(web::WebState* web_state);
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
    url_loader_factory_ = std::move(url_loader_factory);
  }

 private:
  raw_ptr<web::WebState, DanglingUntriaged> web_state_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_API_AI_CHAT_PAGE_CONTENT_FETCHER_H_
