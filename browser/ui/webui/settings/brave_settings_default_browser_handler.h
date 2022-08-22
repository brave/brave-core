/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_DEFAULT_BROWSER_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_DEFAULT_BROWSER_HANDLER_H_

#include "chrome/browser/ui/webui/settings/settings_default_browser_handler.h"

namespace settings {

class BraveDefaultBrowserHandler : public DefaultBrowserHandler {
 public:
  using DefaultBrowserHandler::DefaultBrowserHandler;
  BraveDefaultBrowserHandler(const BraveDefaultBrowserHandler&) = delete;
  BraveDefaultBrowserHandler& operator=(const BraveDefaultBrowserHandler&) =
      delete;
  ~BraveDefaultBrowserHandler() override;

  // DefaultBrowserHandler overrides:
  void SetAsDefaultBrowser(const base::Value::List& args) override;
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_DEFAULT_BROWSER_HANDLER_H_
