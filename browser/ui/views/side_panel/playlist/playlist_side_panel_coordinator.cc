// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"

#include <memory>

#include "base/callback.h"
#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_web_view.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/views/vector_icons.h"

PlaylistSidePanelCoordinator::PlaylistSidePanelCoordinator(Browser* browser)
    : BrowserUserData<PlaylistSidePanelCoordinator>(*browser) {}

PlaylistSidePanelCoordinator::~PlaylistSidePanelCoordinator() = default;

void PlaylistSidePanelCoordinator::CreateAndRegisterEntry(
    SidePanelRegistry* global_registry) {
  global_registry->Register(std::make_unique<SidePanelEntry>(
      SidePanelEntry::Id::kPlaylist,
      l10n_util::GetStringUTF16(IDS_READ_LATER_TITLE), ui::ImageModel(),
      base::BindRepeating(&PlaylistSidePanelCoordinator::CreateWebView,
                          base::Unretained(this))));
}

std::unique_ptr<views::View> PlaylistSidePanelCoordinator::CreateWebView() {
  return std::make_unique<PlaylistSidePanelWebView>(&GetBrowser(),
                                                    base::RepeatingClosure());
}

BROWSER_USER_DATA_KEY_IMPL(PlaylistSidePanelCoordinator);
