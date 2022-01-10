// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <utility>

#include "brave/browser/ui/webui/brave_shields/shields_data_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

ShieldsDataHandler::ShieldsDataHandler(
    mojo::PendingReceiver<brave_shields_panel::mojom::DataHandler>
        data_handler_receiver,
    ui::MojoBubbleWebUIController* webui_controller)
    : data_handler_receiver_(this, std::move(data_handler_receiver)),
      webui_controller_(webui_controller) {
  auto* profile = Profile::FromWebUI(webui_controller_->web_ui());
  DCHECK(profile);
  Browser* browser = chrome::FindLastActiveWithProfile(profile);
  browser->tab_strip_model()->AddObserver(this);
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  if (!shields_data_ctrlr)
    return;
  UpdateSiteBlockInfo();
  shields_data_ctrlr->AddObserver(this);
}

ShieldsDataHandler::~ShieldsDataHandler() {
  /* The lifecycle of this class is similar to ShieldsPanelUI and
   * ShieldsPanelUI's cache gets destryed after ~300ms of being idle.
   */
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  if (!shields_data_ctrlr)
    return;
  shields_data_ctrlr->RemoveObserver(this);
}

void ShieldsDataHandler::RegisterUIHandler(
    mojo::PendingRemote<brave_shields_panel::mojom::UIHandler>
        ui_handler_receiver) {
  ui_handler_remote_.Bind(std::move(ui_handler_receiver));
  UpdateSiteBlockInfo();
}

void ShieldsDataHandler::GetSiteBlockInfo(GetSiteBlockInfoCallback callback) {
  std::move(callback).Run(site_block_info_.Clone());
}

BraveShieldsDataController*
ShieldsDataHandler::GetActiveShieldsDataController() {
  auto* profile = Profile::FromWebUI(webui_controller_->web_ui());
  DCHECK(profile);

  Browser* browser = chrome::FindLastActiveWithProfile(profile);
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();

  if (web_contents) {
    return BraveShieldsDataController::FromWebContents(web_contents);
  }

  return nullptr;
}

void ShieldsDataHandler::UpdateSiteBlockInfo() {
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  if (!shields_data_ctrlr)
    return;

  site_block_info_.host = shields_data_ctrlr->GetCurrentSiteURL().host();
  site_block_info_.total_blocked_resources =
      shields_data_ctrlr->GetTotalBlockedCount();

  // Notify remote that data changed
  if (ui_handler_remote_) {
    ui_handler_remote_.get()->OnSiteBlockInfoChanged(site_block_info_.Clone());
  }
}

void ShieldsDataHandler::OnResourcesChanged() {
  UpdateSiteBlockInfo();
}

void ShieldsDataHandler::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    // OnResourcesChanged doesnt get triggered instantly on active tab change so
    // trigger this explicitly
    UpdateSiteBlockInfo();

    if (selection.new_contents) {
      BraveShieldsDataController::FromWebContents(selection.new_contents)
          ->AddObserver(this);
    }

    if (selection.old_contents) {
      BraveShieldsDataController::FromWebContents(selection.old_contents)
          ->RemoveObserver(this);
    }
  }
}
