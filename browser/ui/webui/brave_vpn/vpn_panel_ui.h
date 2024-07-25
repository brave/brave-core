/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_VPN_PANEL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_VPN_PANEL_UI_H_

#include <memory>
#include <string>

#include "brave/browser/ui/webui/brave_vpn/vpn_panel_handler.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/untrusted_web_ui_controller.h"

// In the style of TopChromeWebUIController but for UntrustedWebUI instead
class VPNPanelUI : public ui::UntrustedWebUIController,
                   public brave_vpn::mojom::PanelHandlerFactory {
 public:
  using Embedder = TopChromeWebUIController::Embedder;

  explicit VPNPanelUI(content::WebUI* web_ui);
  VPNPanelUI(const VPNPanelUI&) = delete;
  VPNPanelUI& operator=(const VPNPanelUI&) = delete;
  ~VPNPanelUI() override;

  // Instantiates the implementor of the mojom::PanelHandlerFactory mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<brave_vpn::mojom::PanelHandlerFactory> receiver);

  // From TopChromeWebUIController
  void set_embedder(base::WeakPtr<Embedder> embedder) { embedder_ = embedder; }
  base::WeakPtr<Embedder> embedder() { return embedder_; }

  static constexpr std::string GetWebUIName() { return "VPNPanel"; }

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
  // From TopChromeWebUIController
  base::WeakPtr<Embedder> embedder_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class UntrustedVPNPanelUIConfig
    : public DefaultTopChromeWebUIConfig<VPNPanelUI> {
 public:
  UntrustedVPNPanelUIConfig();
  ~UntrustedVPNPanelUIConfig() override = default;

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
  bool ShouldAutoResizeHost() override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_VPN_VPN_PANEL_UI_H_
