/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_web_view.h"

PlaylistSidePanelWebView::PlaylistSidePanelWebView(
    Browser* browser,
    SidePanelEntryScope& scope,
    base::RepeatingClosure close_cb,
    WebUIContentsWrapper* contents_wrapper)
    : SidePanelWebUIView(scope,
                         /* on_show_cb = */ base::RepeatingClosure(),
                         close_cb,
                         contents_wrapper) {}

PlaylistSidePanelWebView::~PlaylistSidePanelWebView() = default;

base::WeakPtr<PlaylistSidePanelWebView> PlaylistSidePanelWebView::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}
