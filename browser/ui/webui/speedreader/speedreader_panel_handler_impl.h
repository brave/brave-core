// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_HANDLER_IMPL_H_
#define BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_HANDLER_IMPL_H_

#include "brave/components/speedreader/common/speedreader_panel.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

class SpeedreaderPanelHandlerImpl : public speedreader::mojom::PanelHandler {
 public:
  SpeedreaderPanelHandlerImpl(
      mojo::PendingReceiver<speedreader::mojom::PanelHandler> receiver,
      ui::MojoBubbleWebUIController* webui_controller);

  SpeedreaderPanelHandlerImpl(const SpeedreaderPanelHandlerImpl&) = delete;
  SpeedreaderPanelHandlerImpl& operator=(const SpeedreaderPanelHandlerImpl&) =
      delete;

  ~SpeedreaderPanelHandlerImpl() override;

  // speedreader::mojom::PanelHandler overrides
  void ShowBubble() override;
  void CloseBubble() override;

 private:
  mojo::Receiver<speedreader::mojom::PanelHandler> receiver_;
  raw_ptr<ui::MojoBubbleWebUIController> const webui_controller_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_PANEL_HANDLER_IMPL_H_
