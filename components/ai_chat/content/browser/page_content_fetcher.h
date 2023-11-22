// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PAGE_CONTENT_FETCHER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PAGE_CONTENT_FETCHER_H_

#include <string>

#include "base/functional/callback_forward.h"

namespace content {
class WebContents;
}  // namespace content

namespace ai_chat {

using FetchPageContentCallback =
    base::OnceCallback<void(std::string, bool is_video)>;

void FetchPageContent(content::WebContents* web_contents,
                      FetchPageContentCallback callback);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PAGE_CONTENT_FETCHER_H_
