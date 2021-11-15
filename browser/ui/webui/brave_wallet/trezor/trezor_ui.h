/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_TREZOR_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_TREZOR_UI_H_

#include <memory>

#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/untrusted_web_ui_controller.h"
#include "ui/webui/webui_config.h"

namespace trezor {

class UntrustedTrezorUI : public ui::UntrustedWebUIController {
 public:
  explicit UntrustedTrezorUI(content::WebUI* web_ui);
  UntrustedTrezorUI(const UntrustedTrezorUI&) = delete;
  UntrustedTrezorUI& operator=(const UntrustedTrezorUI&) = delete;
  ~UntrustedTrezorUI() override;
};

class UntrustedTrezorUIConfig : public ui::WebUIConfig {
 public:
  UntrustedTrezorUIConfig();
  ~UntrustedTrezorUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui) override;
};

}  // namespace trezor

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_TREZOR_TREZOR_UI_H_
