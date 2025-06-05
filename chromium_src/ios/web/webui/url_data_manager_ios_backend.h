// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_URL_DATA_MANAGER_IOS_BACKEND_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_URL_DATA_MANAGER_IOS_BACKEND_H_

template <typename T>
class BraveProtocolHandler;

#define CreateProtocolHandler                                        \
  CreateProtocolHandler(BrowserState* browser_state);                \
                                                                     \
  template <typename T>                                              \
  friend class BraveProtocolHandler;                                 \
                                                                     \
  static std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler> \
      CreateProtocolHandler_ChromiumImpl

#include "src/ios/web/webui/url_data_manager_ios_backend.h"  // IWYU pragma: export

#undef CreateProtocolHandler

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_URL_DATA_MANAGER_IOS_BACKEND_H_
