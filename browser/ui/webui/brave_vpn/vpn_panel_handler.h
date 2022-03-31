// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_VPN_PANEL_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_VPN_PANEL_HANDLER_H_

#include <string>
#include <vector>

#include "brave/components/brave_vpn/brave_vpn.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

namespace content {
class WebUI;
}  // namespace content

class BraveVpnServiceDesktop;

class VPNPanelHandler : public brave_vpn::mojom::PanelHandler {
 public:
  using GetWebContentsForTabCallback =
      base::RepeatingCallback<content::WebContents*(int32_t)>;

  VPNPanelHandler(
      mojo::PendingReceiver<brave_vpn::mojom::PanelHandler> receiver,
      ui::MojoBubbleWebUIController* webui_controller);

  VPNPanelHandler(const VPNPanelHandler&) = delete;
  VPNPanelHandler& operator=(const VPNPanelHandler&) = delete;
  ~VPNPanelHandler() override;

  // brave_vpn::mojom::PanelHandler:
  void ShowUI() override;
  void CloseUI() override;

 private:
  mojo::Receiver<brave_vpn::mojom::PanelHandler> receiver_;
  ui::MojoBubbleWebUIController* const webui_controller_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_VPN_PANEL_HANDLER_H_
