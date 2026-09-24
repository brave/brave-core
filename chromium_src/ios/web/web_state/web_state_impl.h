// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_WEB_STATE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_WEB_STATE_IMPL_H_

namespace web {
class WebUIIOS;
}

// Exposes an API to obtain the main frame WebUI which is required for some
// Brave WebUI implementations
#define ClearWebUI                    \
  ClearWebUI();                       \
  web::WebUIIOS* GetMainFrameWebUI(); \
  size_t GetWebUICountForTesting

#include <ios/web/web_state/web_state_impl.h>  // IWYU pragma: export

#undef ClearWebUI

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_WEB_STATE_IMPL_H_
