/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_URL_HANDLER_H_
#define BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_URL_HANDLER_H_

namespace content {
class BrowserContext;
}

class GURL;

namespace ai_chat {

bool HandleURLReverseRewrite(GURL* url,
                             content::BrowserContext* browser_context);
bool HandleURLRewrite(GURL* url, content::BrowserContext* browser_context);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_URL_HANDLER_H_
