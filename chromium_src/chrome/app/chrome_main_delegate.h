/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_APP_CHROME_MAIN_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_APP_CHROME_MAIN_DELEGATE_H_

#include "brave/common/brave_content_client.h"
#include "chrome/common/chrome_content_client.h"
#include "content/public/app/content_main_delegate.h"

#define ChromeContentClient BraveContentClient

#define BasicStartupComplete           \
  BasicStartupComplete_ChromiumImpl(); \
  absl::optional<int> BasicStartupComplete

#include "src/chrome/app/chrome_main_delegate.h"

#undef BasicStartupComplete
#undef ChromeContentClient

#endif  // BRAVE_CHROMIUM_SRC_CHROME_APP_CHROME_MAIN_DELEGATE_H_
