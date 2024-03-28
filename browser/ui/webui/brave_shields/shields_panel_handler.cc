// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_shields/shields_panel_handler.h"

#include <utility>

#include "brave/browser/ui/brave_browser_window.h"
#include "brave/components/brave_shields/content/browser/brave_shields_p3a.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "ui/gfx/geometry/vector2d.h"

ShieldsPanelHandler::ShieldsPanelHandler(
    mojo::PendingReceiver<brave_shields::mojom::PanelHandler> receiver,
    TopChromeWebUIController* webui_controller,
    BraveBrowserWindow* brave_browser_window,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      webui_controller_(webui_controller),
      brave_browser_window_(brave_browser_window),
      profile_(profile) {}

ShieldsPanelHandler::~ShieldsPanelHandler() = default;

void ShieldsPanelHandler::ShowUI() {
  auto embedder = webui_controller_->embedder();
  if (embedder) {
    embedder->ShowUI();
  }
  brave_shields::MaybeRecordShieldsUsageP3A(
      brave_shields::ShieldsIconUsage::kClicked,
      g_browser_process->local_state());
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

void ShieldsPanelHandler::SetAdvancedViewEnabled(bool is_enabled) {
  DCHECK(profile_);

  profile_->GetPrefs()->SetBoolean(kShieldsAdvancedViewEnabled, is_enabled);
}

void ShieldsPanelHandler::GetAdvancedViewEnabled(
    GetAdvancedViewEnabledCallback callback) {
  DCHECK(profile_);

  bool is_enabled =
      profile_->GetPrefs()->GetBoolean(kShieldsAdvancedViewEnabled);
  std::move(callback).Run(is_enabled);
}
