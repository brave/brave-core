/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_SERVICE_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_SERVICE_FACTORY_H_

// Below files are included in advance to prevent overriding
// GetBrowserContextToUse.
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

#define GetBrowserContextToUse GetBrowserContextToUse_ChromiumImpl(content::BrowserContext* context) const; content::BrowserContext* GetBrowserContextToUse  // NOLINT

#include "src/chrome/browser/themes/theme_service_factory.h"

#undef GetBrowserContextToUse

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_THEMES_THEME_SERVICE_FACTORY_H_
