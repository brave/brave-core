/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_UI_H_

#include <memory>

#include "brave/components/brave_rewards/common/mojom/rewards_tip_panel.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

namespace brave_rewards {

class TipPanelUI : public ui::MojoBubbleWebUIController,
                   public mojom::TipPanelHandlerFactory {
 public:
  explicit TipPanelUI(content::WebUI* web_ui);
  ~TipPanelUI() override;

  TipPanelUI(const TipPanelUI&) = delete;
  TipPanelUI& operator=(const TipPanelUI&) = delete;

  void BindInterface(mojo::PendingReceiver<TipPanelHandlerFactory> receiver);

 private:
  // mojom::TipPanelHandlerFactory:
  void CreateHandler(
      mojo::PendingRemote<mojom::TipPanel> panel,
      mojo::PendingReceiver<mojom::TipPanelHandler> handler) override;

  std::unique_ptr<mojom::TipPanelHandler> handler_;
  mojo::Receiver<TipPanelHandlerFactory> factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_TIP_PANEL_UI_H_
