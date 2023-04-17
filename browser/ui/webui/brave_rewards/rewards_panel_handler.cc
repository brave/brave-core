/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_panel_handler.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/models/simple_menu_model.h"

namespace {

// Provides the context menu for the Rewards panel.
class RewardsPanelContextMenuModel : public ui::SimpleMenuModel,
                                     public ui::SimpleMenuModel::Delegate {
 public:
  RewardsPanelContextMenuModel(content::WebContents* web_contents,
                               int32_t x,
                               int32_t y)
      : ui::SimpleMenuModel(this), web_contents_(web_contents), x_(x), y_(y) {
    AddItemWithStringId(IDC_CONTENT_CONTEXT_INSPECTELEMENT,
                        IDS_CONTENT_CONTEXT_INSPECTELEMENT);
  }

  RewardsPanelContextMenuModel(const RewardsPanelContextMenuModel&) = delete;
  RewardsPanelContextMenuModel& operator=(const RewardsPanelContextMenuModel&) =
      delete;

  ~RewardsPanelContextMenuModel() override = default;

 private:
  // ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override {
    if (command_id == IDC_CONTENT_CONTEXT_INSPECTELEMENT) {
      ExecInspectElement();
    }
  }

  void ExecInspectElement() {
    auto* rfh = web_contents_->GetFocusedFrame();
    if (!rfh) {
      return;
    }
    DevToolsWindow::InspectElement(rfh, x_, y_);
  }

  raw_ptr<content::WebContents> web_contents_ = nullptr;
  int32_t x_ = 0;
  int32_t y_ = 0;
};

}  // namespace

RewardsPanelHandler::RewardsPanelHandler(
    mojo::PendingRemote<brave_rewards::mojom::Panel> panel,
    mojo::PendingReceiver<brave_rewards::mojom::PanelHandler> receiver,
    content::WebContents* web_contents,
    base::WeakPtr<ui::MojoBubbleWebUIController::Embedder> embedder,
    brave_rewards::RewardsService* rewards_service,
    brave_rewards::RewardsPanelCoordinator* panel_coordinator)
    : receiver_(this, std::move(receiver)),
      panel_(std::move(panel)),
      web_contents_(web_contents),
      embedder_(embedder),
      rewards_service_(rewards_service),
      panel_coordinator_(panel_coordinator) {
  DCHECK(web_contents_);
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
    if (panel_coordinator_) {
      panel_coordinator_->CloseOnDeactivate(true);
    }
    embedder_->CloseUI();
  }
}

void RewardsPanelHandler::ShowContextMenu(int32_t x, int32_t y) {
  if (embedder_) {
    if (panel_coordinator_) {
      panel_coordinator_->CloseOnDeactivate(false);
    }
    embedder_->ShowContextMenu(
        gfx::Point(x, y),
        std::make_unique<RewardsPanelContextMenuModel>(web_contents_, x, y));
  }
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
