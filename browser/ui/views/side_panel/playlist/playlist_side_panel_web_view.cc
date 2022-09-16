/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_web_view.h"

#include <memory>

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/bubble/bubble_contents_wrapper.h"

PlaylistSidePanelWebView::PlaylistSidePanelWebView(
    Browser* browser,
    base::RepeatingClosure close_cb)
    : SidePanelWebUIViewT(
          /* on_show_cb = */ base::RepeatingClosure(),
          close_cb,
          std::make_unique<BubbleContentsWrapperT<playlist::PlaylistUI>>(
              GURL(kPlaylistURL),
              browser->profile(),
              0,
              /*webui_resizes_host=*/false,
              /*esc_closes_ui=*/false)) {}

PlaylistSidePanelWebView::~PlaylistSidePanelWebView() = default;
