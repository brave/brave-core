/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_shields/shields_panel_peek_provider.h"

#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

ShieldsPanelPeekProvider::ShieldsPanelPeekProvider() = default;

ShieldsPanelPeekProvider::~ShieldsPanelPeekProvider() = default;

std::unique_ptr<WebUIBubbleManager>
ShieldsPanelPeekProvider::CreateBubbleManager(BrowserWindowInterface& browser) {
  return WebUIBubbleManager::Create<ShieldsPanelUI>(
      &browser, GURL(kShieldsPanelURL), IDS_BRAVE_SHIELDS);
}

void ShieldsPanelPeekProvider::SetPanelTarget(
    content::WebContents* panel_contents,
    content::WebContents* target) {
  ShieldsPanelUI::SetTargetWebContents(panel_contents, target);
}
