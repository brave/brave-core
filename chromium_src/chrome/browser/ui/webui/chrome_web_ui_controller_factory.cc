/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_web_ui_controller_factory.h"
#include "build/chromeos_buildflags.h"

#if !BUILDFLAG(IS_CHROMEOS_ASH)
// NOLINTNEXTLINE
#define CHROME_BROWSER_WEB_APPLICATIONS_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_MANAGER_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#endif  // !BUILDFLAG(IS_CHROMEOS_ASH)

#define BRAVE_CHROME_WEBUI_CONTROLLER_FACTORY \
  return BraveWebUIControllerFactory::GetInstance();

#include "src/chrome/browser/ui/webui/chrome_web_ui_controller_factory.cc"
#undef BRAVE_CHROME_WEBUI_CONTROLLER_FACTORY
