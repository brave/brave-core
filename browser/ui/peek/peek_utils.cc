/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/peek/peek_utils.h"

#include "content/public/browser/page_navigator.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace peek {

bool IsPeekableURL(const GURL& url) {
  return url.is_valid() && url.SchemeIsHTTPOrHTTPS();
}

bool IsShiftClickOpen(const content::OpenURLParams& params) {
  return params.disposition == WindowOpenDisposition::NEW_WINDOW &&
         params.is_renderer_initiated && params.user_gesture &&
         IsPeekableURL(params.url);
}

}  // namespace peek
