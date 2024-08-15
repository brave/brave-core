// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SEARCHBOX_REALBOX_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SEARCHBOX_REALBOX_HANDLER_H_

#define GetMatchWithUrl(...)    \
  GetMatchWithUrl(__VA_ARGS__); \
                                \
  friend class BraveRealboxHandlerTest

#include "src/chrome/browser/ui/webui/searchbox/realbox_handler.h"  // IWYU pragma: export

#undef GetMatchWithUrl

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SEARCHBOX_REALBOX_HANDLER_H_
