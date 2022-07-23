/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_panel_handler.h"

#include <utility>

#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"

RewardsPanelHandler::RewardsPanelHandler(
    mojo::PendingRemote<brave_rewards::mojom::Panel> panel,
    mojo::PendingReceiver<brave_rewards::mojom::PanelHandler> receiver,
    base::WeakPtr<ui::MojoBubbleWebUIController::Embedder> embedder,
    brave_rewards::RewardsService* rewards_service,
    brave_rewards::RewardsPanelCoordinator* panel_coordinator)
    : receiver_(this, std::move(receiver)),
      panel_(std::move(panel)),
      embedder_(embedder),
      rewards_service_(rewards_service),
      panel_coordinator_(panel_coordinator) {
  DCHECK(embedder_);
  DCHECK(rewards_service_);
  if (panel_coordinator_) {
    panel_observation_.Observe(panel_coordinator_);
  }
}

RewardsPanelHandler::~RewardsPanelHandler() = default;

void RewardsPanelHandler::ShowUI() {
  if (embedder_) {
    embedder_->ShowUI();
  }
}

void RewardsPanelHandler::CloseUI() {
  if (embedder_) {
    embedder_->CloseUI();
  }
}

void RewardsPanelHandler::StartRewards(StartRewardsCallback callback) {
  if (!rewards_service_) {
    std::move(callback).Run();
    return;
  }

  rewards_service_->StartProcess(std::move(callback));
}

void RewardsPanelHandler::GetRewardsPanelArgs(
    GetRewardsPanelArgsCallback callback) {
  std::move(callback).Run(panel_coordinator_
                              ? panel_coordinator_->panel_args().Clone()
                              : brave_rewards::mojom::RewardsPanelArgs::New());
}

void RewardsPanelHandler::OnRewardsPanelRequested(
    const brave_rewards::mojom::RewardsPanelArgs& args) {
  panel_->OnRewardsPanelRequested(args.Clone());
}
