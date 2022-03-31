/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_VPN_PANEL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_VPN_PANEL_UI_H_

#include <memory>

#include "brave/browser/ui/webui/brave_vpn/vpn_panel_handler.h"
#include "brave/components/brave_vpn/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

class VPNPanelUI : public ui::MojoBubbleWebUIController,
                   public brave_vpn::mojom::PanelHandlerFactory {
 public:
  explicit VPNPanelUI(content::WebUI* web_ui);
  VPNPanelUI(const VPNPanelUI&) = delete;
  VPNPanelUI& operator=(const VPNPanelUI&) = delete;
  ~VPNPanelUI() override;

  // Instantiates the implementor of the mojom::PanelHandlerFactory mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<brave_vpn::mojom::PanelHandlerFactory> receiver);

 private:
  // brave_vpn::mojom::PanelHandlerFactory:
  void CreatePanelHandler(
      mojo::PendingRemote<brave_vpn::mojom::Page> page,
      mojo::PendingReceiver<brave_vpn::mojom::PanelHandler> panel_receiver,
      mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler>
          vpn_service_receiver) override;

  std::unique_ptr<VPNPanelHandler> panel_handler_;

  mojo::Receiver<brave_vpn::mojom::PanelHandlerFactory> panel_factory_receiver_{
      this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_VPN_PANEL_UI_H_
