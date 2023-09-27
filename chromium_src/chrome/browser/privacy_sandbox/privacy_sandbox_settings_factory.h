/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRIVACY_SANDBOX_PRIVACY_SANDBOX_SETTINGS_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRIVACY_SANDBOX_PRIVACY_SANDBOX_SETTINGS_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

#define BuildServiceInstanceForBrowserContext                                  \
  BuildServiceInstanceForBrowserContext_ChromiumImpl(content::BrowserContext*) \
      const;                                                                   \
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext

#include "src/chrome/browser/privacy_sandbox/privacy_sandbox_settings_factory.h"  // IWYU pragma: export

#undef BuildServiceInstanceForBrowserContext

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRIVACY_SANDBOX_PRIVACY_SANDBOX_SETTINGS_FACTORY_H_
