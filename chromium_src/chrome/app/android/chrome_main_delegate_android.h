/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_APP_ANDROID_CHROME_MAIN_DELEGATE_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_CHROME_APP_ANDROID_CHROME_MAIN_DELEGATE_ANDROID_H_

#include "brave/app/brave_main_delegate.h"

#define ChromeMainDelegate BraveMainDelegate
#include "../../../../../chrome/app/android/chrome_main_delegate_android.h"
#undef ChromeMainDelegate

#endif  // BRAVE_CHROMIUM_SRC_CHROME_APP_ANDROID_CHROME_MAIN_DELEGATE_ANDROID_H_
