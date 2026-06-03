// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_WEB_APPS_FRAME_TOOLBAR_WEB_APP_TOOLBAR_BUTTON_CONTAINER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_WEB_APPS_FRAME_TOOLBAR_WEB_APP_TOOLBAR_BUTTON_CONTAINER_H_

#include "ui/views/view.h"

#define AddedToWidget()         \
  AddedToWidget_ChromiumImpl(); \
  void AddedToWidget()

#include <chrome/browser/ui/views/web_apps/frame_toolbar/web_app_toolbar_button_container.h>  // IWYU pragma: export

#undef AddedToWidget

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_WEB_APPS_FRAME_TOOLBAR_WEB_APP_TOOLBAR_BUTTON_CONTAINER_H_
