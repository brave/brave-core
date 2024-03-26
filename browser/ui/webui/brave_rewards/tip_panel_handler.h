/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/common/mojom/rewards_tip_panel.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class Profile;

namespace brave_rewards {

class RewardsService;

class TipPanelHandler : public mojom::TipPanelHandler {
 public:
  TipPanelHandler(mojo::PendingRemote<mojom::TipPanel> banner,
                  mojo::PendingReceiver<mojom::TipPanelHandler> receiver,
                  base::WeakPtr<TopChromeWebUIController::Embedder> embedder,
                  Profile* profile);

  TipPanelHandler(const TipPanelHandler&) = delete;
  TipPanelHandler& operator=(const TipPanelHandler&) = delete;

  ~TipPanelHandler() override;

  // mojom::TipPanelHandler:
  void ShowUI() override;

  void CloseUI() override;

  void GetBrowserSize(GetBrowserSizeCallback callback) override;

  void GetRewardsParameters(GetRewardsParametersCallback callback) override;

  void GetBalance(GetBalanceCallback callback) override;

  void GetBanner(GetBannerCallback callback) override;

  void GetMonthlyContributionSet(
      GetMonthlyContributionSetCallback callback) override;

  void GetExternalWallet(GetExternalWalletCallback callback) override;

  void SendContribution(double amount,
                        bool set_monthly,
                        SendContributionCallback callback) override;

  void OpenTab(const std::string& url) override;

 private:
  mojo::Receiver<mojom::TipPanelHandler> receiver_;
  mojo::Remote<mojom::TipPanel> banner_;
  base::WeakPtr<TopChromeWebUIController::Embedder> embedder_;
  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<RewardsService> rewards_service_ = nullptr;
  std::string publisher_id_;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_HANDLER_H_
