// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_DATA_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_DATA_HANDLER_H_

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
using brave_shields_panel::mojom::SiteBlockInfo;

class ShieldsDataHandler : public brave_shields_panel::mojom::DataHandler,
                           public BraveShieldsDataController::Observer,
                           public TabStripModelObserver {
 public:
  ShieldsDataHandler(
      mojo::PendingReceiver<brave_shields_panel::mojom::DataHandler>
          data_handler_receiver,
      ui::MojoBubbleWebUIController* webui_controller);

  ShieldsDataHandler(const ShieldsDataHandler&) = delete;
  ShieldsDataHandler& operator=(const ShieldsDataHandler&) = delete;
  ~ShieldsDataHandler() override;

  // mojom::DataHandler
  void RegisterUIHandler(
      mojo::PendingRemote<brave_shields_panel::mojom::UIHandler>
          ui_handler_receiver) override;
  void GetSiteBlockInfo(GetSiteBlockInfoCallback callback) override;

 private:
  BraveShieldsDataController* GetActiveShieldsDataController();
  void UpdateSiteBlockInfo();

  // brave_shields::BraveShieldsDataController
  void OnResourcesChanged() override;
  // TabStripModelObserver
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  mojo::Receiver<brave_shields_panel::mojom::DataHandler>
      data_handler_receiver_;
  mojo::Remote<brave_shields_panel::mojom::UIHandler> ui_handler_remote_;
  ui::MojoBubbleWebUIController* const webui_controller_;
  SiteBlockInfo site_block_info_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_DATA_HANDLER_H_
