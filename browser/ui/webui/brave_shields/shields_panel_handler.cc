// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_shields/shields_panel_handler.h"

#include <utility>

#include "brave/browser/ui/brave_browser_window.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"

ShieldsPanelHandler::ShieldsPanelHandler(
    mojo::PendingReceiver<brave_shields::mojom::PanelHandler> receiver,
    ui::MojoBubbleWebUIController* webui_controller,
    BraveBrowserWindow* brave_browser_window)
    : receiver_(this, std::move(receiver)),
      webui_controller_(webui_controller),
      brave_browser_window_(brave_browser_window) {}

ShieldsPanelHandler::~ShieldsPanelHandler() = default;

void ShieldsPanelHandler::ShowUI() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->ShowUI();
  }
}

void ShieldsPanelHandler::CloseUI() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->CloseUI();
  }
}

void ShieldsPanelHandler::GetPosition(GetPositionCallback callback) {
  gfx::Vector2d vec =
      gfx::Vector2d(brave_browser_window_->GetShieldsBubbleRect().x(),
                    brave_browser_window_->GetShieldsBubbleRect().y());
  std::move(callback).Run(vec);
}
