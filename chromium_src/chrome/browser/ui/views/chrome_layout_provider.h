// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_CHROME_LAYOUT_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_CHROME_LAYOUT_PROVIDER_H_

#define CreateLayoutProvider             \
    CreateLayoutProvider_ChromiumImpl(); \
    static std::unique_ptr<views::LayoutProvider> CreateLayoutProvider
#include "src/chrome/browser/ui/views/chrome_layout_provider.h"
#undef CreateLayoutProvider

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_CHROME_LAYOUT_PROVIDER_H_
