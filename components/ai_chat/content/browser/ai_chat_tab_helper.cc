/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/ai_chat/content/browser/associated_web_contents_content.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "pdf/buildflags.h"

#if BUILDFLAG(ENABLE_PDF)
#include "components/pdf/browser/pdf_document_helper.h"
#endif  // BUILDFLAG(ENABLE_PDF)

namespace ai_chat {

DEFINE_USER_DATA(AIChatTabHelper);

// static
AIChatTabHelper* AIChatTabHelper::From(tabs::TabInterface* tab) {
  return tab ? Get(tab->GetUnownedUserDataHost()) : nullptr;
}

AIChatTabHelper::AIChatTabHelper(tabs::TabInterface& tab,
                                 PrintPreviewExtractionDelegateFactory
                                     print_preview_extraction_delegate_factory)
    : print_preview_extraction_delegate_factory_(
          std::move(print_preview_extraction_delegate_factory)),
      web_contents_content_(CreateWebContentsContent(tab.GetContents())),
      will_discard_contents_subscription_(tab.RegisterWillDiscardContents(
          base::BindRepeating(&AIChatTabHelper::OnDiscardContents,
                              base::Unretained(this)))),
      scoped_unowned_user_data_(tab.GetUnownedUserDataHost(), *this),
      tab_(tab) {}

AIChatTabHelper::~AIChatTabHelper() = default;

content::WebContents& AIChatTabHelper::GetWebContents() const {
  return *tab_->GetContents();
}

std::unique_ptr<AssociatedWebContentsContent>
AIChatTabHelper::CreateWebContentsContent(content::WebContents* web_contents) {
  return std::make_unique<AssociatedWebContentsContent>(
      web_contents,
      print_preview_extraction_delegate_factory_
          ? print_preview_extraction_delegate_factory_.Run(web_contents)
          : nullptr);
}

void AIChatTabHelper::OnDiscardContents(tabs::TabInterface* tab,
                                        content::WebContents* old_contents,
                                        content::WebContents* new_contents) {
  // The new content is in place before the old one tells its observers that
  // it is destroyed.
  web_contents_content_ = CreateWebContentsContent(new_contents);
}

}  // namespace ai_chat
