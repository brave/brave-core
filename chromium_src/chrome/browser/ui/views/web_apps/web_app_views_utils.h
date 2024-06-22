// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_WEB_APPS_WEB_APP_VIEWS_UTILS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_WEB_APPS_WEB_APP_VIEWS_UTILS_H_

#define CreateOriginLabelFromStartUrl                               \
  CreateOriginLabelFromStartUrl_ChromiumImpl(const GURL& start_url, \
                                             bool is_primary_text); \
  std::unique_ptr<views::Label> CreateOriginLabelFromStartUrl

#include "src/chrome/browser/ui/views/web_apps/web_app_views_utils.h"  // IWYU pragma: export

#undef CreateOriginLabelFromStartUrl

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_WEB_APPS_WEB_APP_VIEWS_UTILS_H_
