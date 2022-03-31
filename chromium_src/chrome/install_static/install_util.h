/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_INSTALL_UTIL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_INSTALL_UTIL_H_

#define GetChromeChannel           \
  GetChromeChannel_ChromiumImpl(); \
  version_info::Channel GetChromeChannel

#include "src/chrome/install_static/install_util.h"
#undef GetChromeChannel

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_INSTALL_UTIL_H_
