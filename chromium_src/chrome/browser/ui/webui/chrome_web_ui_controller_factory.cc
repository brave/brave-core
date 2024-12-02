/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_web_ui_controller_factory.h"

// Needed since we define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
// below
#include "chrome/browser/web_applications/web_app_provider.h"

// NOLINTNEXTLINE
#define CHROME_BROWSER_WEB_APPLICATIONS_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_MANAGER_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_

#define BRAVE_CHROME_WEBUI_CONTROLLER_FACTORY \
  return BraveWebUIControllerFactory::GetInstance();

#include "src/chrome/browser/ui/webui/chrome_web_ui_controller_factory.cc"
#undef BRAVE_CHROME_WEBUI_CONTROLLER_FACTORY
#undef CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#undef CHROME_BROWSER_WEB_APPLICATIONS_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_MANAGER_H_
