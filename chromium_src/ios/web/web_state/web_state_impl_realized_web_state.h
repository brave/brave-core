// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_WEB_STATE_IMPL_REALIZED_WEB_STATE_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_WEB_STATE_IMPL_REALIZED_WEB_STATE_H_

#include <map>

#include "ios/web/web_state/web_state_impl.h"

// The goal of this override is to introduce a new map of WebUIIOS so that
// WebStateImpl can handle WebUI in multiple frames in one page. It introduces
// a set of replacements for the WebUI method implementations to use said map
//
// The reason we need to allow multiple WebUI's per frame is because some of our
// WebUI applications such as AI Chat use subframes to host chrome-untrusted
// frames
#define web_ui_                                                                \
  web_ui_;                                                                     \
  std::map<std::string, std::unique_ptr<web::WebUIIOS>, std::less<>> web_uis_; \
  void TearDown_ChromiumImpl();                                                \
  void CreateWebUI_ChromiumImpl(const GURL& url);                              \
  void ClearWebUI_ChromiumImpl();                                              \
  bool HasWebUI_ChromiumImpl() const;                                          \
  void HandleWebUIMessage_ChromiumImpl(const GURL&, std::string_view,          \
                                       const base::ListValue&)
// Exposes an API to obtain the main frame WebUI which is required for some
// Brave WebUI implementations
#define ClearWebUI                    \
  ClearWebUI();                       \
  web::WebUIIOS* GetMainFrameWebUI(); \
  size_t GetWebUICountForTesting

#include <ios/web/web_state/web_state_impl_realized_web_state.h>  // IWYU pragma: export

#undef ClearWebUI
#undef web_ui_

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_WEB_STATE_IMPL_REALIZED_WEB_STATE_H_
