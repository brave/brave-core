// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_BROWSER_STATE_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_BROWSER_STATE_H_

#include "base/notreached.h"

class PrefService;

// This override is tied to the replaced implementation of `//ios/web_view`'s
// CWVWebViewConfiguration (see cwv_web_view_configuration_internal.h override)
//
// Since this replaced implementation replaces WebViewBrowserState with
// web::BrowserState, there are a few places that use the GetPrefs method
// exposed on WebViewBrowserState.
//
// This adds a `GetPrefs` method to the public web::BrowserState definition
// to avoid build failures, even though the code that is calling it will never
// be used by us.

#define GetURLLoaderFactory() \
  GetURLLoaderFactory();      \
  PrefService* GetPrefs() {   \
    NOTREACHED();             \
    return nullptr;           \
  }                           \
  void Unused()
#include "src/ios/web/public/browser_state.h"  // IWYU pragma: export
#undef GetURLLoaderFactory

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_BROWSER_STATE_H_
