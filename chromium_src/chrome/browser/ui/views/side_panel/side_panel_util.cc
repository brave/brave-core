// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/side_panel/side_panel_util.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "brave/components/ai_chat/common/buildflags/buildflags.h"
#include "brave/components/playlist/common/features.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_coordinator.h"
#include "brave/components/ai_chat/common/features.h"
#endif

#define PopulateGlobalEntries PopulateGlobalEntries_ChromiumImpl

#include "src/chrome/browser/ui/views/side_panel/side_panel_util.cc"

#undef PopulateGlobalEntries

// static
void SidePanelUtil::PopulateGlobalEntries(Browser* browser,
                                          SidePanelRegistry* global_registry) {
  PopulateGlobalEntries_ChromiumImpl(browser, global_registry);

  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    PlaylistSidePanelCoordinator::GetOrCreateForBrowser(browser)
        ->CreateAndRegisterEntry(global_registry);
  }

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (ai_chat::features::IsAIChatEnabled()) {
    AIChatSidePanelCoordinator::GetOrCreateForBrowser(browser)
        ->CreateAndRegisterEntry(global_registry);
  }
#endif
}

// static
std::string SidePanelUtil::GetHistogramNameForId(SidePanelEntry::Id id) {
  if (id == SidePanelEntry::Id::kPlaylist)
    return "Brave.Playlist";

  if (id == SidePanelEntry::Id::kChatUI) {
    return "Brave.AIChat";
  }

  return ::GetHistogramNameForId(id);
}
