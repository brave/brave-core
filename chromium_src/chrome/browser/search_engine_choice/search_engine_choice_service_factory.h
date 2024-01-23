/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SEARCH_ENGINE_CHOICE_SEARCH_ENGINE_CHOICE_SERVICE_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SEARCH_ENGINE_CHOICE_SEARCH_ENGINE_CHOICE_SERVICE_FACTORY_H_

#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "content/public/browser/browser_context.h"

#define BuildServiceInstanceForBrowserContext           \
  BuildServiceInstanceForBrowserContext_unused();       \
  content::BrowserContext* GetBrowserContextToUse(      \
      content::BrowserContext* context) const override; \
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext

#include "src/chrome/browser/search_engine_choice/search_engine_choice_service_factory.h"  // IWYU pragma: export

#undef BuildServiceInstanceForBrowserContext

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SEARCH_ENGINE_CHOICE_SEARCH_ENGINE_CHOICE_SERVICE_FACTORY_H_
