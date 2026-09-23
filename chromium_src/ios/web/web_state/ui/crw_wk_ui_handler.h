// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_CRW_WK_UI_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_CRW_WK_UI_HANDLER_H_

#include <ios/web/web_state/ui/crw_wk_ui_handler.h>  // IWYU pragma: export

// A CRWWKUIHandler subclass which will override the WKUIDelegate methods to
// handle JavaScript prompts
@interface BraveCRWWKUIHandler : CRWWKUIHandler
@end

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_CRW_WK_UI_HANDLER_H_
