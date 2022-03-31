/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_IO_DATA_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_IO_DATA_H_

#include <string>
#include "url/gurl.h"

#define IsHandledProtocol                                    \
  IsHandledProtocol_ChromiumImpl(const std::string& scheme); \
  static bool IsHandledProtocol

#define IsHandledURL                          \
  IsHandledURL_ChromiumImpl(const GURL& url); \
  static bool IsHandledURL

#include "src/chrome/browser/profiles/profile_io_data.h"

#undef IsHandledURL
#undef IsHandledProtocol

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_IO_DATA_H_
