/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_UI_H_

#include <memory>
#include <string>

#include "brave/components/brave_rewards/common/mojom/rewards_tip_panel.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace content {
class BrowserContext;
}

namespace brave_rewards {

class TipPanelUI : public TopChromeWebUIController,
                   public mojom::TipPanelHandlerFactory {
 public:
  explicit TipPanelUI(content::WebUI* web_ui);
  ~TipPanelUI() override;

  TipPanelUI(const TipPanelUI&) = delete;
  TipPanelUI& operator=(const TipPanelUI&) = delete;

  void BindInterface(mojo::PendingReceiver<TipPanelHandlerFactory> receiver);

  static constexpr std::string GetWebUIName() { return "TipPanel"; }

 private:
  // mojom::TipPanelHandlerFactory:
  void CreateHandler(
      mojo::PendingRemote<mojom::TipPanel> panel,
      mojo::PendingReceiver<mojom::TipPanelHandler> handler) override;

  std::unique_ptr<mojom::TipPanelHandler> handler_;
  mojo::Receiver<TipPanelHandlerFactory> factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class TipPanelUIConfig : public DefaultTopChromeWebUIConfig<TipPanelUI> {
 public:
  TipPanelUIConfig();

  // WebUIConfig::
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  // TopChromeWebUIConfig::
  bool ShouldAutoResizeHost() override;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_UI_H_
