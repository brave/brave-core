/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/screenshot/content/pdf_utils.h"

#include "content/public/browser/web_contents.h"

namespace screenshot {

bool IsPdf(content::WebContents* web_contents) {
  return web_contents->GetContentsMimeType() == "application/pdf";
}

}  // namespace screenshot
