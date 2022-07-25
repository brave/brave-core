// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/speedreader/speedreader_panel_handler_impl.h"

#include <utility>

#include "chrome/browser/profiles/profile.h"

SpeedreaderPanelHandlerImpl::SpeedreaderPanelHandlerImpl(
    mojo::PendingReceiver<speedreader::mojom::PanelHandler> receiver,
    ui::MojoBubbleWebUIController* webui_controller)
    : receiver_(this, std::move(receiver)),
      webui_controller_(webui_controller) {}

SpeedreaderPanelHandlerImpl::~SpeedreaderPanelHandlerImpl() = default;

void SpeedreaderPanelHandlerImpl::ShowBubble() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->ShowUI();
  }
}

void SpeedreaderPanelHandlerImpl::CloseBubble() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->CloseUI();
  }
}
