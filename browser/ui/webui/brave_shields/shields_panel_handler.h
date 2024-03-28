// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_HANDLER_H_

#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

class TopChromeWebUIController;

namespace content {
class WebUI;
}  // namespace content

class Profile;
class BraveBrowserWindow;

class ShieldsPanelHandler : public brave_shields::mojom::PanelHandler {
 public:
  ShieldsPanelHandler(
      mojo::PendingReceiver<brave_shields::mojom::PanelHandler> receiver,
      TopChromeWebUIController* webui_controller,
      BraveBrowserWindow* brave_browser_window,
      Profile* profile);

  ShieldsPanelHandler(const ShieldsPanelHandler&) = delete;
  ShieldsPanelHandler& operator=(const ShieldsPanelHandler&) = delete;
  ~ShieldsPanelHandler() override;

  // brave_shields::mojom::PanelHandler:
  void ShowUI() override;
  void CloseUI() override;
  void GetPosition(GetPositionCallback callback) override;
  void SetAdvancedViewEnabled(bool is_enabled) override;
  void GetAdvancedViewEnabled(GetAdvancedViewEnabledCallback callback) override;

 private:
  mojo::Receiver<brave_shields::mojom::PanelHandler> receiver_;
  raw_ptr<TopChromeWebUIController> const webui_controller_;
  raw_ptr<BraveBrowserWindow> brave_browser_window_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_HANDLER_H_
