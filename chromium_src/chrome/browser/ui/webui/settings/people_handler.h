/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_PEOPLE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_PEOPLE_HANDLER_H_
#define BRAVE_PEOPLE_HANDLER_H_                            \
 private:                                                  \
  void HandleGetDeviceList(const base::ListValue* args);   \
  void HandleGetSyncCode(const base::ListValue* args);     \
  void HandleSetSyncCode(const base::ListValue* args);     \
  void HandleReset(const base::ListValue* args);
#include "../../../../../../../chrome/browser/ui/webui/settings/people_handler.h"
#undef BRAVE_PEOPLE_HANDLER_H_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_PEOPLE_HANDLER_H_
