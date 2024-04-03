/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_FACTORY_H_

#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "content/public/browser/browser_context.h"

#define ServiceIsNULLWhileTesting                       \
  ServiceIsNULLWhileTesting_unused() {                  \
    return false;                                       \
  }                                                     \
  content::BrowserContext* GetBrowserContextToUse(      \
      content::BrowserContext* context) const override; \
  bool ServiceIsNULLWhileTesting

#include "src/chrome/browser/search_engines/template_url_service_factory.h"  // IWYU pragma: export

#undef ServiceIsNULLWhileTesting

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_SERVICE_FACTORY_H_
