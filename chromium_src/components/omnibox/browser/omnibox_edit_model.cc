/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/omnibox/browser/omnibox_edit_model.h"

#include "base/memory/raw_ptr.h"
#include "components/omnibox/browser/omnibox_controller.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
#include "brave/components/commander/common/constants.h"
#include "brave/components/commander/common/features.h"
#endif

#if !BUILDFLAG(IS_IOS)
#include "content/public/common/url_constants.h"
#endif

namespace {
void BraveAdjustTextForCopy(GURL* url) {
#if !BUILDFLAG(IS_IOS)
  if (url->scheme() == content::kChromeUIScheme) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kBraveUIScheme);
    *url = url->ReplaceComponents(replacements);
  }
#endif
}

}  // namespace

#define BRAVE_ADJUST_TEXT_FOR_COPY BraveAdjustTextForCopy(url_from_text);

#define CanPasteAndGo CanPasteAndGo_Chromium
#include "src/components/omnibox/browser/omnibox_edit_model.cc"
#undef CanPasteAndGo
#undef BRAVE_ADJUST_TEXT_FOR_COPY

bool OmniboxEditModel::CanPasteAndGo(const std::u16string& text) const {
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  if (commander::CommanderEnabled() &&
      base::StartsWith(text, commander::kCommandPrefix)) {
    return false;
  }
#endif
  return CanPasteAndGo_Chromium(text);
}
