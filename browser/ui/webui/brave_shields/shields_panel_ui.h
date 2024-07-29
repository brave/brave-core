/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_UI_H_

#include <memory>
#include <string>

#include "brave/browser/ui/webui/brave_shields/shields_panel_data_handler.h"
#include "brave/browser/ui/webui/brave_shields/shields_panel_handler.h"
#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

class Browser;

namespace content {
class BrowserContext;
}

class ShieldsPanelUI : public TopChromeWebUIController,
                       public brave_shields::mojom::PanelHandlerFactory {
 public:
  explicit ShieldsPanelUI(content::WebUI* web_ui);
  ShieldsPanelUI(const ShieldsPanelUI&) = delete;
  ShieldsPanelUI& operator=(const ShieldsPanelUI&) = delete;
  ~ShieldsPanelUI() override;

  // Instantiates the implementor of the mojom::PanelHandlerFactory mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<brave_shields::mojom::PanelHandlerFactory>
          receiver);

  static constexpr std::string GetWebUIName() { return "ShieldsPanel"; }

 private:
  void CreatePanelHandler(
      mojo::PendingReceiver<brave_shields::mojom::PanelHandler> panel_receiver,
      mojo::PendingReceiver<brave_shields::mojom::DataHandler>
          data_handler_receiver) override;

  std::unique_ptr<ShieldsPanelHandler> panel_handler_;
  std::unique_ptr<ShieldsPanelDataHandler> data_handler_;

  mojo::Receiver<brave_shields::mojom::PanelHandlerFactory>
      panel_factory_receiver_{this};

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<Browser> browser_ = nullptr;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class ShieldsPanelUIConfig
    : public DefaultTopChromeWebUIConfig<ShieldsPanelUI> {
 public:
  ShieldsPanelUIConfig();

  // WebUIConfig::
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  // TopChromeWebUIConfig::
  bool ShouldAutoResizeHost() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_UI_H_
