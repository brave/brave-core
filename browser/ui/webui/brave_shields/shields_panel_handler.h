// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_HANDLER_H_

#include "brave/components/brave_shields/common/brave_shields_panel.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace ui {
class MojoBubbleWebUIController;
}  // namespace ui

namespace content {
class WebUI;
}  // namespace content

class ShieldsPanelHandler : public brave_shields::mojom::PanelHandler {
 public:
  ShieldsPanelHandler(
      mojo::PendingReceiver<brave_shields::mojom::PanelHandler> receiver,
      ui::MojoBubbleWebUIController* webui_controller);

  ShieldsPanelHandler(const ShieldsPanelHandler&) = delete;
  ShieldsPanelHandler& operator=(const ShieldsPanelHandler&) = delete;
  ~ShieldsPanelHandler() override;

  // brave_shields::mojom::PanelHandler:
  void ShowUI() override;
  void CloseUI() override;

 private:
  mojo::Receiver<brave_shields::mojom::PanelHandler> receiver_;
  ui::MojoBubbleWebUIController* const webui_controller_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_SHIELDS_SHIELDS_PANEL_HANDLER_H_
