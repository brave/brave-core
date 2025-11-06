// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_shields/shields_panel_handler.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "components/prefs/pref_service.h"
#include "ui/gfx/geometry/vector2d.h"

ShieldsPanelHandler::ShieldsPanelHandler(
    mojo::PendingReceiver<brave_shields::mojom::PanelHandler> receiver,
    TopChromeWebUIController* webui_controller,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      webui_controller_(webui_controller),
      profile_(profile) {
  CHECK(profile_);
  CHECK(webui_controller_);
}

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
  auto* browser_window_interface = webui::GetBrowserWindowInterface(
      webui_controller_->web_ui()->GetWebContents());
  if (!browser_window_interface) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  auto const* browser = browser_window_interface->GetBrowserForMigrationOnly();
  if (!browser || !browser->window()) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  auto* browser_window = BraveBrowserWindow::From(browser->window());
  auto rect = browser_window->GetShieldsBubbleRect();
  if (rect.IsEmpty()) {
    // If the browser cannot determine the location of the Shields bubble
    // (e.g. the WebUI is being displayed in the page info bubble), then use
    // an arbitrary point that leaves enough room for the other controls in
    // the bubble.
    std::move(callback).Run(gfx::Vector2d(0, 64));
    return;
  }
  std::move(callback).Run(gfx::Vector2d(rect.x(), rect.y()));
}

void ShieldsPanelHandler::SetAdvancedViewEnabled(bool is_enabled) {
  CHECK(profile_);

  profile_->GetPrefs()->SetBoolean(kShieldsAdvancedViewEnabled, is_enabled);
}

void ShieldsPanelHandler::GetAdvancedViewEnabled(
    GetAdvancedViewEnabledCallback callback) {
  CHECK(profile_);

  bool is_enabled =
      profile_->GetPrefs()->GetBoolean(kShieldsAdvancedViewEnabled);
  std::move(callback).Run(is_enabled);
}
