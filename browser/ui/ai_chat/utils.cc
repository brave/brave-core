/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/ai_chat/utils.h"

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/public/tab_interface.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_id.h"

namespace ai_chat {

void OpenAIChatForTab(content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  auto* tab = tabs::TabInterface::GetFromContents(web_contents);
  auto* coordinator =
      tab->GetBrowserWindowInterface()->GetFeatures().side_panel_coordinator();
  CHECK(coordinator);
  coordinator->Show(SidePanelEntryId::kChatUI);
}

}  // namespace ai_chat
