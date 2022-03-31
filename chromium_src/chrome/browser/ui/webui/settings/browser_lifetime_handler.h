/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_BROWSER_LIFETIME_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_BROWSER_LIFETIME_HANDLER_H_

#define BrowserLifetimeHandler BrowserLifetimeHandler_ChromiumImpl
#define HandleRelaunch \
  Dummy();             \
                       \
 protected:            \
  virtual void HandleRelaunch

#include "src/chrome/browser/ui/webui/settings/browser_lifetime_handler.h"
#undef HandleRelaunch
#undef BrowserLifetimeHandler

namespace settings {

class BrowserLifetimeHandler : public BrowserLifetimeHandler_ChromiumImpl {
 public:
  using BrowserLifetimeHandler_ChromiumImpl::
      BrowserLifetimeHandler_ChromiumImpl;

  BrowserLifetimeHandler(const BrowserLifetimeHandler&) = delete;
  BrowserLifetimeHandler& operator=(const BrowserLifetimeHandler&) = delete;

  ~BrowserLifetimeHandler() override;

 private:
  void HandleRelaunch(base::Value::ConstListView args) override;
};

}  // namespace settings

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SETTINGS_BROWSER_LIFETIME_HANDLER_H_
