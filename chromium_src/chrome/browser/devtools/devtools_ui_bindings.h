// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DEVTOOLS_DEVTOOLS_UI_BINDINGS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DEVTOOLS_DEVTOOLS_UI_BINDINGS_H_

#define IsValidRemoteFrontendURL \
    IsValidRemoteFrontendURL_ChromiumImpl(const GURL& url); \
    static bool IsValidRemoteFrontendURL
#include "../../../../../chrome/browser/devtools/devtools_ui_bindings.h"
#undef IsValidRemoteFrontendURL

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DEVTOOLS_DEVTOOLS_UI_BINDINGS_H_
