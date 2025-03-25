/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_WEB_UI_MOCHA_BROWSER_TEST_H_
#define BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_WEB_UI_MOCHA_BROWSER_TEST_H_

#include "base/test/scoped_feature_list.h"
#include "components/webui/chrome_urls/features.h"

#define coverage_handler_                             \
  coverage_handler_unused_;                           \
  base::test::ScopedFeatureList scoped_feature_list_{ \
      chrome_urls::kInternalOnlyUisPref};             \
  std::unique_ptr<DevToolsAgentCoverageObserver> coverage_handler_

#include "src/chrome/test/base/web_ui_mocha_browser_test.h"  // IWYU pragma: export

#undef coverage_handler_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_WEB_UI_MOCHA_BROWSER_TEST_H_
