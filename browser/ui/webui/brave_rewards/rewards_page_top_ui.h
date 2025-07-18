/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_TOP_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_TOP_UI_H_

#include <memory>
#include <string>

#include "brave/components/brave_rewards/core/mojom/rewards_page.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace content {
class BrowserContext;
}

namespace brave_rewards {

// The WebUI controller for the Rewards page when embedded in the Rewards panel.
class RewardsPageTopUI : public TopChromeWebUIController {
 public:
  explicit RewardsPageTopUI(content::WebUI* web_ui);
  ~RewardsPageTopUI() override;

  void BindInterface(mojo::PendingReceiver<mojom::RewardsPageHandler> receiver);

  static constexpr std::string_view GetWebUIName() { return "RewardsPanel"; }

 private:
  std::unique_ptr<mojom::RewardsPageHandler> handler_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class RewardsPageTopUIConfig
    : public DefaultTopChromeWebUIConfig<RewardsPageTopUI> {
 public:
  RewardsPageTopUIConfig();

  // WebUIConfig::
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  // TopChromeWebUIConfig::
  bool ShouldAutoResizeHost() override;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_TOP_UI_H_
