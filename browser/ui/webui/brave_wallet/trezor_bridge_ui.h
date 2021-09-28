/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_UI_H_

#include <string>

#include "content/public/browser/web_ui_controller.h"

class TrezorBridgeUI : public content::WebUIController {
 public:
  TrezorBridgeUI(content::WebUI* web_ui, const std::string& host);
  ~TrezorBridgeUI() override;
  TrezorBridgeUI(const TrezorBridgeUI&) = delete;
  TrezorBridgeUI& operator=(const TrezorBridgeUI&) = delete;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_BRIDGE_UI_H_
