/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_

#include <memory>

#include "base/callback_list.h"
#include "base/functional/callback.h"
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
// Owned by the tab's features. When the tab's contents is replaced, the
// content of the old contents is destroyed and replaced by the new one's.
class AIChatTabHelper {
 public:
  DECLARE_USER_DATA(AIChatTabHelper);

  // Creates the print preview extraction delegate for a contents. Provided as
  // its implementation is in a different layer. May be null.
  using PrintPreviewExtractionDelegateFactory =
      base::RepeatingCallback<std::unique_ptr<
          AssociatedWebContentsContent::PrintPreviewExtractionDelegate>(
          content::WebContents*)>;

  static AIChatTabHelper* From(tabs::TabInterface* tab);

  AIChatTabHelper(tabs::TabInterface& tab,
                  PrintPreviewExtractionDelegateFactory
                      print_preview_extraction_delegate_factory);
  AIChatTabHelper(const AIChatTabHelper&) = delete;
  AIChatTabHelper& operator=(const AIChatTabHelper&) = delete;
  ~AIChatTabHelper();

  // The tab's contents.
  content::WebContents& GetWebContents() const;

  AssociatedWebContentsContent& web_contents_content() const {
    CHECK(web_contents_content_);
    return *web_contents_content_;
  }

 private:
  std::unique_ptr<AssociatedWebContentsContent> CreateWebContentsContent(
      content::WebContents* web_contents);

  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents);

  PrintPreviewExtractionDelegateFactory
      print_preview_extraction_delegate_factory_;
  std::unique_ptr<AssociatedWebContentsContent> web_contents_content_;
  base::CallbackListSubscription will_discard_contents_subscription_;

  ui::ScopedUnownedUserData<AIChatTabHelper> scoped_unowned_user_data_;

  const raw_ref<tabs::TabInterface> tab_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_
