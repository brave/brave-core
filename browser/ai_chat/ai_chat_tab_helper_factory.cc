/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ai_chat/ai_chat_tab_helper_factory.h"

#include "brave/browser/ai_chat/ai_chat_utils.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "printing/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/unguessable_token.h"
#include "brave/browser/ai_chat/print_preview_extraction_delegate_impl.h"
#include "brave/browser/screenshot/print_preview_extractor_factory.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#endif  // BUILDFLAG(ENABLE_PRINT_PREVIEW)

namespace ai_chat {

std::unique_ptr<AIChatTabHelper> MaybeCreateAIChatTabHelper(
    tabs::TabInterface& tab,
    content::WebContents* web_contents) {
  if (!IsAllowedForContext(web_contents->GetBrowserContext())) {
    return nullptr;
  }
  return std::make_unique<AIChatTabHelper>(
      tab, web_contents,
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
      std::make_unique<PrintPreviewExtractionDelegateImpl>(
          web_contents,
          screenshot::CreatePrintPreviewExtractor(base::BindRepeating(
              []() -> base::flat_map<base::UnguessableToken, int>& {
                return printing::PrintPreviewUI::
                    GetPrintPreviewUIRequestIdMap();
              })))
#else
      nullptr
#endif
  );
}

}  // namespace ai_chat
