/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BOOKMARKS_BOOKMARK_MODEL_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BOOKMARKS_BOOKMARK_MODEL_FACTORY_H_

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

#define ServiceIsNULLWhileTesting                       \
  ServiceIsNULLWhileTesting_unused();                   \
  content::BrowserContext* GetBrowserContextToUse(      \
      content::BrowserContext* context) const override; \
  bool ServiceIsNULLWhileTesting

#include "src/chrome/browser/bookmarks/bookmark_model_factory.h"

#undef ServiceIsNULLWhileTesting

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_BOOKMARKS_BOOKMARK_MODEL_FACTORY_H_
