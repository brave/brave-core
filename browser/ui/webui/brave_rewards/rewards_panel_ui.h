/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PANEL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PANEL_UI_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_rewards/common/mojom/rewards_panel.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace content {
class BrowserContext;
}

namespace brave_rewards {

class RewardsPanelCoordinator;

class RewardsPanelUI : public TopChromeWebUIController,
                       public mojom::PanelHandlerFactory {
 public:
  explicit RewardsPanelUI(content::WebUI* web_ui);
  ~RewardsPanelUI() override;

  RewardsPanelUI(const RewardsPanelUI&) = delete;
  RewardsPanelUI& operator=(const RewardsPanelUI&) = delete;

  void BindInterface(mojo::PendingReceiver<PanelHandlerFactory> receiver);

  static constexpr std::string GetWebUIName() { return "RewardsPanel"; }

 private:
  // mojom::PanelHandlerFactory:
  void CreatePanelHandler(
      mojo::PendingRemote<mojom::Panel> panel,
      mojo::PendingReceiver<mojom::PanelHandler> receiver) override;

  std::unique_ptr<mojom::PanelHandler> panel_handler_;
  mojo::Receiver<PanelHandlerFactory> panel_factory_receiver_{this};
  raw_ptr<RewardsPanelCoordinator> panel_coordinator_ = nullptr;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class RewardsPanelUIConfig
    : public DefaultTopChromeWebUIConfig<RewardsPanelUI> {
 public:
  RewardsPanelUIConfig();

  // WebUIConfig::
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  // TopChromeWebUIConfig::
  bool ShouldAutoResizeHost() override;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PANEL_UI_H_
