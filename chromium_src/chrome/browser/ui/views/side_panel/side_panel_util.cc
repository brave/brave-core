// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/side_panel/side_panel_util.h"

#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_web_view.h"
#endif

#define PopulateGlobalEntries PopulateGlobalEntries_ChromiumImpl
#include <chrome/browser/ui/views/side_panel/side_panel_util.cc>
#undef PopulateGlobalEntries

// static
void SidePanelUtil::PopulateGlobalEntries(Browser* browser,
                                          SidePanelRegistry* global_registry) {
  PopulateGlobalEntries_ChromiumImpl(browser, global_registry);

  // the playlist coordinator is not created for popup windows, or for
  // desktop PWAs.
  if (auto* playlist_coordinator =
          browser->GetFeatures().playlist_side_panel_coordinator()) {
    playlist_coordinator->CreateAndRegisterEntry(global_registry);
  }

#if BUILDFLAG(ENABLE_AI_CHAT)
  // AI Chat side panel as a global panel and not tab-specific is conditional
  // for now.
  // TODO(https://github.com/brave/brave-browser/issues/48526): Remove the
  // condition when the feature flag is removed.
  if (ai_chat::AIChatServiceFactory::GetForBrowserContext(browser->profile()) &&
      ai_chat::ShouldSidePanelBeGlobal(browser->profile())) {
    global_registry->Register(std::make_unique<SidePanelEntry>(
        SidePanelEntry::Key(SidePanelEntry::Id::kChatUI),
        base::BindRepeating(&AIChatSidePanelWebView::CreateView,
                            browser->profile(),
                            /*is_tab_associated=*/false),
        base::NullCallback()));
  }
#endif
}
