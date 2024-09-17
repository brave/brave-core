/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_DEVTOOLS_PROTOCOL_NETWORK_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_DEVTOOLS_PROTOCOL_NETWORK_HANDLER_H_

#define NavigationRequestWillBeSent(...)    \
  NavigationRequestWillBeSent(__VA_ARGS__); \
  void RequestAdblockInfoReceived(          \
      const std::string& request_id,        \
      std::unique_ptr<protocol::Network::AdblockInfo> info)

#include "src/content/browser/devtools/protocol/network_handler.h"  // IWYU pragma: export

#undef NavigationRequestWillBeSent

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_DEVTOOLS_PROTOCOL_NETWORK_HANDLER_H_
