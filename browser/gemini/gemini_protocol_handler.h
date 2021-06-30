/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_GEMINI_GEMINI_PROTOCOL_HANDLER_H_
#define BRAVE_BROWSER_GEMINI_GEMINI_PROTOCOL_HANDLER_H_

#include <string>

#include "content/public/browser/web_contents.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace gemini {

void HandleGeminiProtocol(const GURL& url,
                          content::WebContents::OnceGetter web_contents_getter,
                          ui::PageTransition page_transition,
                          bool has_user_gesture,
                          const absl::optional<url::Origin>& initiator);

bool IsGeminiProtocol(const GURL& url);

}  // namespace gemini

#endif  // BRAVE_BROWSER_GEMINI_GEMINI_PROTOCOL_HANDLER_H_
