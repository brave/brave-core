/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/page_action/brave_page_action_icon_type.h"
#include "brave/browser/ui/views/location_bar/brave_star_view.h"
#include "brave/browser/ui/views/speedreader/speedreader_icon_view.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/browser/ui/views/reader_mode/reader_mode_icon_view.h"

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
constexpr bool kSupportsPlaylistActionIconView = true;
#else
constexpr bool kSupportsPlaylistActionIconView = false;
#endif

// Circumvent creation of CookieControlsIconView in
// PageActionIconController::Init's switch statement by injecting a case
// with a non-existent value created above.
#define kCookieControls                                     \
  kCookieControls:                                          \
  break;                                                    \
  case brave::kPlaylistPageActionIconType:                  \
    if constexpr (kSupportsPlaylistActionIconView) {        \
      playlist_action_icon_view_ = add_page_action_icon(    \
          type, std::make_unique<PlaylistActionIconView>(   \
                    params.command_updater, params.browser, \
                    params.icon_label_bubble_delegate,      \
                    params.page_action_icon_delegate));     \
    }                                                       \
    break;                                                  \
  case brave::kUndefinedPageActionIconType

#define ReaderModeIconView SpeedreaderIconView
#define StarView BraveStarView
#include "src/chrome/browser/ui/views/page_action/page_action_icon_controller.cc"
#undef StarView
#undef ReaderModeIconView
#undef kCookieControls

PageActionIconView* PageActionIconController::GetPlaylistActionIconView() {
  return playlist_action_icon_view_.get();
}
