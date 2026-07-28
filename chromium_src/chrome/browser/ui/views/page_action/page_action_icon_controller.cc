/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/page_action/page_action_icon_controller.h"

#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_container.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_params.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_icon_container_view.h"
#include "ui/views/animation/ink_drop.h"

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/browser/ui/views/page_action/wayback_machine_action_icon_view.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/ui/views/speedreader/speedreader_icon_view.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/browser/ui/views/playlist/playlist_action_icon_view.h"
#endif

// Mirrors the `add_page_action_icon` lambda that upstream used to declare
// locally in `Init()`, removed along with `kOptimizationGuide` (its last
// caller) in cr152. The Speedreader/Wayback Machine/Playlist cases plastered
// into `Init()`'s switch (see the matching .cc.yaml) call this to register
// their icon views.
PageActionIconView* PageActionIconController::AddPageActionIcon(
    PageActionIconType type,
    std::unique_ptr<PageActionIconView> icon,
    const PageActionIconParams& params) {
  icon->SetVisible(false);
  views::InkDrop::Get(icon.get())
      ->SetVisibleOpacity(params.page_action_icon_delegate
                              ->GetPageActionInkDropVisibleOpacity());
  if (params.icon_color) {
    icon->SetIconColor(*params.icon_color);
  }
  if (params.font_list) {
    icon->SetFontList(*params.font_list);
  }
  icon->AddPageIconViewObserver(this);
  auto* icon_ptr = icon.get();
  if (params.button_observer) {
    params.button_observer->ObserveButton(icon_ptr);
  }
  icon_container_->AddPageActionIcon(std::move(icon));
  page_action_icon_views_.emplace(type, icon_ptr);
  return icon_ptr;
}

#include <chrome/browser/ui/views/page_action/page_action_icon_controller.cc>

PageActionIconView* PageActionIconController::GetPlaylistActionIconView() {
  return playlist_action_icon_view_.get();
}
