/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"

#include <memory>

#include "base/strings/string_util.h"
#include "brave/components/ai_chat/content/browser/associated_web_contents.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/ai_chat/content/browser/pdf_utils.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "pdf/buildflags.h"

#if BUILDFLAG(ENABLE_PDF)
#include "components/pdf/browser/pdf_document_helper.h"
#endif  // BUILDFLAG(ENABLE_PDF)

namespace ai_chat {

AIChatTabHelper::AIChatTabHelper(
    content::WebContents* web_contents,
    std::unique_ptr<AssociatedWebContents::PrintPreviewExtractionDelegate>
        print_preview_extraction_delegate)
    : content::WebContentsUserData<AIChatTabHelper>(*web_contents),
      associated_web_contents_(std::make_unique<AssociatedWebContents>(
          web_contents,
          std::move(print_preview_extraction_delegate))) {}

AIChatTabHelper::~AIChatTabHelper() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);

}  // namespace ai_chat
