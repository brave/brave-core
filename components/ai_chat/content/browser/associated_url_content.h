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
#include "brave/components/ai_chat/content/browser/associated_web_contents_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace ai_chat {

// Represents a link that has been attached to a conversation.
// The link will be loaded asynchronously in a background WebContents when
// |GetContent| is called.
class AssociatedURLContent : public AssociatedWebContentsContent {
 public:
  ~AssociatedURLContent() override;
  AssociatedURLContent(const AssociatedURLContent&) = delete;
  AssociatedURLContent& operator=(const AssociatedURLContent&) = delete;

  static std::unique_ptr<AssociatedURLContent> Create(
      GURL url,
      std::u16string title,
      content::BrowserContext* browser_context,
      std::unique_ptr<PrintPreviewExtractionDelegate>
          print_preview_extraction_delegate,
      base::OnceCallback<void(content::WebContents*)> attach_tab_helpers);

 protected:
  AssociatedURLContent(GURL url,
                       std::u16string title,
                       std::unique_ptr<PrintPreviewExtractionDelegate>
                           print_preview_extraction_delegate,
                       std::unique_ptr<content::WebContents> web_contents);

  void GetPageContent(FetchPageContentCallback callback,
                      std::string_view invalidation_token) override;
  void OnNewPage(int64_t navigation_id) override;

 private:
  friend class TestableAssociatedURLContent;

  std::unique_ptr<content::WebContents> web_contents_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_ASSOCIATED_URL_CONTENT_H_
