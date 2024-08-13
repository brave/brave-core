/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SEARCH_ENGINES_SEARCH_ENGINE_TAB_HELPER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SEARCH_ENGINES_SEARCH_ENGINE_TAB_HELPER_H_

#include "content/public/browser/navigation_entry.h"

#define GenerateKeywordIfNecessary                    \
  NotUsed() {}                                        \
  bool IsFormSubmit(content::NavigationEntry* entry); \
  void GenerateKeywordIfNecessary

#include "brave/components/search_engines/brave_search_engines_pref_names.h"

#include "src/chrome/browser/ui/search_engines/search_engine_tab_helper.h"  // IWYU pragma: export

#undef GenerateKeywordIfNecessary

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SEARCH_ENGINES_SEARCH_ENGINE_TAB_HELPER_H_
