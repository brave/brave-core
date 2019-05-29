/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_UI_H_

#include <string>

#include "brave/browser/ui/webui/basic_ui.h"

class BraveWalletUI : public BasicUI {
 public:
  BraveWalletUI(content::WebUI* web_ui, const std::string& host);
  ~BraveWalletUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveWalletUI);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_UI_H_
