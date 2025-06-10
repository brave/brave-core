// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_WEB_STATE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_WEB_STATE_IMPL_H_

#include "base/observer_list.h"

namespace web {
class WebUIIOS;
}  // namespace web

// Override observers variable so we can add our own list of WebUIs

#define observers_ \
  observers_;      \
  std::vector<std::unique_ptr<web::WebUIIOS>> brave_web_uis_

// Override CancelDialogs function so we can add our own cleanup code
// as well as functions for handling WebUI creation and messages.
// We also add GetMainWebUI to return the WebUI belonging to the main-frame.

#define CancelDialogs                                          \
  CancelDialogs();                                             \
  void TearDownBraveWebUI();                                   \
  void CreateBraveWebUI(const GURL& url);                      \
  bool HasBraveWebUI() const;                                  \
  void HandleBraveWebUIMessage(const GURL& source_url,         \
                               std::string_view message,       \
                               const base::Value::List& args); \
  web::WebUIIOS* GetMainWebUI() const;                         \
  void ClearBraveWebUI

#include <ios/web/web_state/web_state_impl.h>  // IWYU pragma: export

#undef CancelDialogs

#undef observers_

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_WEB_STATE_IMPL_H_
