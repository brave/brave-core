/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/pdf_utils.h"

#include "content/public/browser/web_contents.h"

namespace ai_chat {

bool IsPdf(content::WebContents* web_contents) {
  return web_contents->GetContentsMimeType() == "application/pdf";
}

}  // namespace ai_chat
