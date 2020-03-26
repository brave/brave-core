/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BAT_CHECKOUT_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BAT_CHECKOUT_UI_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"

class BraveCheckoutUI : public ConstrainedWebDialogUI {
 public:
  BraveCheckoutUI(content::WebUI* web_ui, const std::string& host);
  ~BraveCheckoutUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveCheckoutUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BAT_CHECKOUT_UI_H_
