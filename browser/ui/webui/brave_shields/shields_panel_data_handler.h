// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_DATA_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_DATA_HANDLER_H_

#include "brave/browser/ui/brave_shields_data_controller.h"
#include "brave/components/brave_shields/common/brave_shields_panel.mojom.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace ui {
class MojoBubbleWebUIController;
}  // namespace ui

using brave_shields::BraveShieldsDataController;
using brave_shields::mojom::SiteBlockInfo;
using brave_shields::mojom::SiteSettings;
using favicon::FaviconDriver;

class ShieldsPanelDataHandler : public brave_shields::mojom::DataHandler,
                                public BraveShieldsDataController::Observer,
                                public TabStripModelObserver {
 public:
  ShieldsPanelDataHandler(
      mojo::PendingReceiver<brave_shields::mojom::DataHandler>
          data_handler_receiver,
      ui::MojoBubbleWebUIController* webui_controller);

  ShieldsPanelDataHandler(const ShieldsPanelDataHandler&) = delete;
  ShieldsPanelDataHandler& operator=(const ShieldsPanelDataHandler&) = delete;
  ~ShieldsPanelDataHandler() override;

  // mojom::DataHandler
  void RegisterUIHandler(mojo::PendingRemote<brave_shields::mojom::UIHandler>
                             ui_handler_receiver) override;
  void GetSiteBlockInfo(GetSiteBlockInfoCallback callback) override;
  void GetSiteSettings(GetSiteSettingsCallback callback) override;
  void SetAdBlockMode(AdBlockMode callback) override;
  void SetFingerprintMode(FingerprintMode mode) override;
  void SetCookieBlockMode(CookieBlockMode mode) override;
  void SetIsNoScriptsEnabled(bool is_enabled) override;
  void SetHTTPSEverywhereEnabled(bool is_enabled) override;
  void SetBraveShieldsEnabled(bool is_enabled) override;
  void OpenWebCompatWindow() override;

 private:
  BraveShieldsDataController* GetActiveShieldsDataController();
  void UpdateSiteBlockInfo();

  // BraveShieldsDataController::Observer
  void OnResourcesChanged() override;
  void OnFaviconUpdated() override;

  // TabStripModelObserver
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  mojo::Receiver<brave_shields::mojom::DataHandler> data_handler_receiver_;
  mojo::Remote<brave_shields::mojom::UIHandler> ui_handler_remote_;
  ui::MojoBubbleWebUIController* const webui_controller_;
  SiteBlockInfo site_block_info_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_DATA_HANDLER_H_
