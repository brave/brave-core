/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/ai_chat_url_handler.h"
#include "brave/components/constants/webui_url_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

namespace ai_chat {

bool HandleURLRewrite(GURL* url, content::BrowserContext* browser_context) {
  // Used to get chrome://chat to resolve to chrome-untrusted://chat
  if (url->SchemeIs(content::kChromeUIScheme) && url->DomainIs(kChatUIHost)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIUntrustedScheme);
    *url = url->ReplaceComponents(replacements);
    return true;
  }

  // Needed so that HandleURLReverseRewrite will get hit
  if (url->SchemeIs(content::kChromeUIUntrustedScheme) &&
      url->DomainIs(kChatUIHost)) {
    return true;
  }

  return false;
}

bool HandleURLReverseRewrite(GURL* url,
                             content::BrowserContext* browser_context) {
  // Make chrome-untrusted://chat show up as chrome://chat
  if (url->SchemeIs(content::kChromeUIUntrustedScheme) &&
      url->DomainIs(kChatUIHost)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    *url = url->ReplaceComponents(replacements);
    return true;
  }

  return false;
}

}  // namespace ai_chat
