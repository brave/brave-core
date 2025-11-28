// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_ASSOCIATED_URL_CONTENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_ASSOCIATED_URL_CONTENT_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/timer/timer.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
class WebContents;
class NavigationHandle;
}  // namespace content

namespace ai_chat {

class PageContentFetcher;

// Represents a link that has been attached to a conversation.
// The link will be loaded asynchronously in a background WebContents when
// |GetContent| is called.
// **Note:** This class won't work for PDFs or use print preview extraction.
// TODO(https://github.com/brave/brave-browser/issues/49742): Once the logic
// is available in a shared class instead of AIChatTabHelper we should use that
// instead of the just a PageContentFetcher.
class AssociatedURLContent : public AssociatedContentDelegate,
                             public content::WebContentsObserver {
 public:
  AssociatedURLContent(GURL url,
                       std::u16string title,
                       content::BrowserContext* browser_context);
  ~AssociatedURLContent() override;
  AssociatedURLContent(const AssociatedURLContent&) = delete;
  AssociatedURLContent& operator=(const AssociatedURLContent&) = delete;

  // AssociatedContentDelegate
  void GetContent(GetPageContentCallback callback) override;

  content::WebContents* GetWebContentsForTesting() {
    return web_contents_.get();
  }

 private:
  friend class TestableAssociatedURLContent;

  void OnTimeout();
  void OnContentExtractionComplete(std::string content,
                                   bool is_video,
                                   std::string invalidation_token);
  void CompleteWithError(const std::string& error);

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;

  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<PageContentFetcher> content_fetcher_;
  base::OneShotTimer timeout_timer_;
  std::unique_ptr<base::OneShotEvent> content_loaded_event_;

  base::WeakPtrFactory<AssociatedURLContent> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_ASSOCIATED_URL_CONTENT_H_
