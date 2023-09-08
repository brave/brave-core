// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTERNAL_PROTOCOL_EXTERNAL_PROTOCOL_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTERNAL_PROTOCOL_EXTERNAL_PROTOCOL_HANDLER_H_

#define LaunchUrl(...)                 \
  LaunchUrl_ChromiumImpl(__VA_ARGS__); \
  static void LaunchUrl(__VA_ARGS__)

#include "src/chrome/browser/external_protocol/external_protocol_handler.h"  // IWYU pragma: export

#undef LaunchUrl

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTERNAL_PROTOCOL_EXTERNAL_PROTOCOL_HANDLER_H_
