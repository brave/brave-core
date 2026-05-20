/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"

#include <memory>
#include <utility>

#include "brave/components/ai_chat/content/browser/associated_web_contents_content.h"
#include "pdf/buildflags.h"

namespace content {
class WebContents;
}  // namespace content

namespace ai_chat {

AIChatTabHelper::AIChatTabHelper(
    content::WebContents* web_contents,
    std::unique_ptr<
        AssociatedWebContentsContent::PrintPreviewExtractionDelegate>
        print_preview_extraction_delegate)
    : content::WebContentsUserData<AIChatTabHelper>(*web_contents),
      web_contents_content_(std::make_unique<AssociatedWebContentsContent>(
          web_contents,
          std::move(print_preview_extraction_delegate))) {}

AIChatTabHelper::~AIChatTabHelper() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);

}  // namespace ai_chat
