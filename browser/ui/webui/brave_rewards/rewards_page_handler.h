/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/scoped_observation.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/common/mojom/rewards_page.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_rewards {

// The WebUI handler for messages originating from the Rewards page.
class RewardsPageHandler : public mojom::RewardsPageHandler,
                           public RewardsServiceObserver {
 public:
  // An optional helper that can be supplied by WebUI controller to allow
  // the WebUI application to execute bubble actions.
  class BubbleDelegate {
   public:
    virtual ~BubbleDelegate() = default;
    virtual void ShowUI() = 0;
    virtual void OpenTab(const std::string& url) = 0;
  };

  RewardsPageHandler(mojo::PendingRemote<mojom::RewardsPage> page,
                     mojo::PendingReceiver<mojom::RewardsPageHandler> receiver,
                     std::unique_ptr<BubbleDelegate> bubble_delegate,
                     RewardsService* rewards_service);

  ~RewardsPageHandler() override;

  // mojom::RewardsPageHandler:
  void OnPageReady() override;
  void OpenTab(const std::string& url) override;
  void GetAvailableCountries(GetAvailableCountriesCallback callback) override;
  void GetRewardsPaymentId(GetRewardsPaymentIdCallback callback) override;
  void EnableRewards(const std::string& country_code,
                     EnableRewardsCallback callback) override;
  void ResetRewards(ResetRewardsCallback callback) override;

  // RewardsServiceObserver:
  void OnRewardsInitialized(RewardsService* rewards_service) override;
  void OnRewardsWalletCreated() override;
  void OnCompleteReset(bool success) override;

 private:
  mojo::Receiver<mojom::RewardsPageHandler> receiver_;
  mojo::Remote<mojom::RewardsPage> page_;
  std::unique_ptr<BubbleDelegate> bubble_delegate_;
  raw_ptr<RewardsService> rewards_service_ = nullptr;
  base::ScopedObservation<RewardsService, RewardsServiceObserver>
      rewards_observation_{this};
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_HANDLER_H_
