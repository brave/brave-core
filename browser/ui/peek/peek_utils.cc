/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/peek/peek_utils.h"

#include "chrome/app/chrome_command_ids.h"
#include "content/public/browser/page_navigator.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace peek {

bool IsPeekableURL(const GURL& url) {
  return url.is_valid() && url.SchemeIsHTTPOrHTTPS();
}

bool IsPeekableInitiator(const url::Origin& origin) {
  return !origin.opaque() && (origin.scheme() == url::kHttpScheme ||
                              origin.scheme() == url::kHttpsScheme);
}

bool IsShiftClickOpen(const content::OpenURLParams& params) {
  return params.disposition == WindowOpenDisposition::kNewWindow &&
         params.is_renderer_initiated && params.user_gesture &&
         IsPeekableURL(params.url);
}

bool CanOpenInPeek(const content::OpenURLParams& params) {
  return IsPeekableURL(params.url) && params.initiator_origin.has_value() &&
         IsPeekableInitiator(*params.initiator_origin) &&
         params.initiator_navigation_state;
}

bool IsPageCommand(int command_id) {
  switch (command_id) {
    case IDC_BACK:
    case IDC_FORWARD:
    case IDC_RELOAD:
    case IDC_RELOAD_BYPASSING_CACHE:
    case IDC_RELOAD_CLEARING_CACHE:
    case IDC_STOP:
    case IDC_SAVE_PAGE:
    case IDC_PRINT:
    case IDC_BASIC_PRINT:
    case IDC_FIND:
    case IDC_FIND_NEXT:
    case IDC_FIND_PREVIOUS:
    case IDC_ZOOM_PLUS:
    case IDC_ZOOM_NORMAL:
    case IDC_ZOOM_MINUS:
    case IDC_BOOKMARK_THIS_TAB:
    case IDC_VIEW_SOURCE:
    case IDC_DEV_TOOLS:
    case IDC_DEV_TOOLS_CONSOLE:
    case IDC_DEV_TOOLS_INSPECT:
    case IDC_DEV_TOOLS_TOGGLE:
      return true;
    default:
      return false;
  }
}

}  // namespace peek
