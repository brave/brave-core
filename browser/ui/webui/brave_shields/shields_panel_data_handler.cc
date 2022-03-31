// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <utility>

#include "brave/browser/ui/webui/brave_shields/shields_panel_data_handler.h"
#include "brave/browser/webcompat_reporter/webcompat_reporter_dialog.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

ShieldsPanelDataHandler::ShieldsPanelDataHandler(
    mojo::PendingReceiver<brave_shields::mojom::DataHandler>
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

ShieldsPanelDataHandler::~ShieldsPanelDataHandler() {
  /* The lifecycle of this class is similar to ShieldsPanelUI and
   * ShieldsPanelUI's cache gets destryed after ~300ms of being idle.
   */
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  if (!shields_data_ctrlr)
    return;
  shields_data_ctrlr->RemoveObserver(this);
}

void ShieldsPanelDataHandler::RegisterUIHandler(
    mojo::PendingRemote<brave_shields::mojom::UIHandler> ui_handler_receiver) {
  ui_handler_remote_.Bind(std::move(ui_handler_receiver));
  UpdateSiteBlockInfo();
}

void ShieldsPanelDataHandler::GetSiteBlockInfo(
    GetSiteBlockInfoCallback callback) {
  std::move(callback).Run(site_block_info_.Clone());
}

void ShieldsPanelDataHandler::GetSiteSettings(
    GetSiteSettingsCallback callback) {
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  DCHECK(shields_data_ctrlr);

  SiteSettings settings;
  settings.ad_block_mode = shields_data_ctrlr->GetAdBlockMode();
  settings.fingerprint_mode = shields_data_ctrlr->GetFingerprintMode();
  settings.cookie_block_mode = shields_data_ctrlr->GetCookieBlockMode();
  settings.is_https_everywhere_enabled =
      shields_data_ctrlr->GetHTTPSEverywhereEnabled();
  settings.is_noscript_enabled = shields_data_ctrlr->GetNoScriptEnabled();

  std::move(callback).Run(settings.Clone());
}

void ShieldsPanelDataHandler::SetAdBlockMode(AdBlockMode mode) {
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  DCHECK(shields_data_ctrlr);

  shields_data_ctrlr->SetAdBlockMode(mode);
}

void ShieldsPanelDataHandler::SetFingerprintMode(FingerprintMode mode) {
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  DCHECK(shields_data_ctrlr);

  shields_data_ctrlr->SetFingerprintMode(mode);
}

void ShieldsPanelDataHandler::SetCookieBlockMode(CookieBlockMode mode) {
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  DCHECK(shields_data_ctrlr);

  shields_data_ctrlr->SetCookieBlockMode(mode);
}

void ShieldsPanelDataHandler::SetIsNoScriptsEnabled(bool is_enabled) {
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  DCHECK(shields_data_ctrlr);

  shields_data_ctrlr->SetIsNoScriptEnabled(is_enabled);
}

void ShieldsPanelDataHandler::SetHTTPSEverywhereEnabled(bool is_enabled) {
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  DCHECK(shields_data_ctrlr);

  shields_data_ctrlr->SetIsHTTPSEverywhereEnabled(is_enabled);
}

void ShieldsPanelDataHandler::SetBraveShieldsEnabled(bool is_enabled) {
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  DCHECK(shields_data_ctrlr);

  shields_data_ctrlr->SetBraveShieldsEnabled(is_enabled);
}

void ShieldsPanelDataHandler::OpenWebCompatWindow() {
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  DCHECK(shields_data_ctrlr);

  OpenWebcompatReporterDialog(shields_data_ctrlr->web_contents());
}

BraveShieldsDataController*
ShieldsPanelDataHandler::GetActiveShieldsDataController() {
  auto* profile = Profile::FromWebUI(webui_controller_->web_ui());
  DCHECK(profile);

  Browser* browser = chrome::FindLastActiveWithProfile(profile);
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();

  if (web_contents) {
    return BraveShieldsDataController::FromWebContents(web_contents);
  }

  return nullptr;
}

void ShieldsPanelDataHandler::UpdateSiteBlockInfo() {
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  if (!shields_data_ctrlr)
    return;

  site_block_info_.host = shields_data_ctrlr->GetCurrentSiteURL().host();
  site_block_info_.total_blocked_resources =
      shields_data_ctrlr->GetTotalBlockedCount();
  site_block_info_.ads_list = shields_data_ctrlr->GetBlockedAdsList();
  site_block_info_.js_list = shields_data_ctrlr->GetJsList();
  site_block_info_.fingerprints_list =
      shields_data_ctrlr->GetFingerprintsList();
  site_block_info_.http_redirects_list =
      shields_data_ctrlr->GetHttpRedirectsList();
  site_block_info_.is_shields_enabled =
      shields_data_ctrlr->GetBraveShieldsEnabled();

  // This method gets called from various callsites. Constantly updating favicon
  // url will replace the hashed version too. So, we update this once only
  if (site_block_info_.favicon_url.is_empty()) {
    site_block_info_.favicon_url = shields_data_ctrlr->GetFaviconURL(false);
  }

  // Notify remote that data changed
  if (ui_handler_remote_) {
    ui_handler_remote_.get()->OnSiteBlockInfoChanged(site_block_info_.Clone());
  }
}

void ShieldsPanelDataHandler::OnResourcesChanged() {
  UpdateSiteBlockInfo();
}

void ShieldsPanelDataHandler::OnFaviconUpdated() {
  auto* shields_data_ctrlr = GetActiveShieldsDataController();
  if (!shields_data_ctrlr)
    return;

  site_block_info_.favicon_url = shields_data_ctrlr->GetFaviconURL(true);

  // Notify remote that favicon changed
  if (ui_handler_remote_) {
    ui_handler_remote_.get()->OnSiteBlockInfoChanged(site_block_info_.Clone());
  }
}

void ShieldsPanelDataHandler::OnTabStripModelChanged(
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
