/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_APP_LAUNCHER_LOGIN_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_APP_LAUNCHER_LOGIN_HANDLER_H_

#include "content/public/browser/web_ui_message_handler.h"

#define RegisterMessages           \
  RegisterMessages_ChromiumImpl(); \
  virtual void RegisterMessages
#include "../../../../../../chrome/browser/ui/webui/app_launcher_login_handler.h"
#undef RegisterMessages

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_APP_LAUNCHER_LOGIN_HANDLER_H_
