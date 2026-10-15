/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_

#include <memory>

#include "base/memory/raw_ref.h"
#include "brave/components/ai_chat/content/browser/associated_web_contents_content.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace ai_chat {

// Provides context to an AI Chat conversation in the form of the Tab's content.
// Owned by the tab's features, and recreated when the tab's contents is
// replaced.
class AIChatTabHelper {
 public:
  DECLARE_USER_DATA(AIChatTabHelper);

  static AIChatTabHelper* From(tabs::TabInterface* tab);

  // PrintPreviewExtractionDelegate is provided as it's implementation is
  // in a different layer.
  AIChatTabHelper(
      tabs::TabInterface& tab,
      content::WebContents* web_contents,
      std::unique_ptr<
          AssociatedWebContentsContent::PrintPreviewExtractionDelegate>
          print_preview_extraction_delegate);
  AIChatTabHelper(const AIChatTabHelper&) = delete;
  AIChatTabHelper& operator=(const AIChatTabHelper&) = delete;
  ~AIChatTabHelper();

  // The contents this was created for.
  content::WebContents& GetWebContents() const { return *contents_; }

  AssociatedWebContentsContent& web_contents_content() const {
    CHECK(web_contents_content_);
    return *web_contents_content_;
  }

 private:
  const raw_ref<content::WebContents> contents_;
  std::unique_ptr<AssociatedWebContentsContent> web_contents_content_;

  ui::ScopedUnownedUserData<AIChatTabHelper> scoped_unowned_user_data_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_
