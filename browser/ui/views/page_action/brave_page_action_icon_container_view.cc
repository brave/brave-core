/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_action/brave_page_action_icon_container_view.h"

#include "base/check_is_test.h"
#include "brave/browser/ui/page_action/brave_page_action_icon_type.h"
#include "brave/components/brave_player/common/buildflags/buildflags.h"
#include "brave/components/playlist/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_params.h"
#include "ui/base/metadata/metadata_impl_macros.h"

#if BUILDFLAG(ENABLE_BRAVE_PLAYER)
#include "brave/components/brave_player/common/features.h"
#endif

namespace {

PageActionIconParams& ModifyIconParamsForBrave(PageActionIconParams& params) {
  // Add actions for Brave
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    // Browser could be null if the location bar was created for
    // PresentationReceiverWindowView.
    if (params.browser && params.browser->is_type_normal() &&
        !params.browser->profile()->IsOffTheRecord()) {
      // Insert Playlist action before sharing hub or at the end of the vector.
      params.types_enabled.insert(
          base::ranges::find(params.types_enabled,
                             PageActionIconType::kSharingHub),
          brave::kPlaylistPageActionIconType);
    }
  }

#if BUILDFLAG(ENABLE_BRAVE_PLAYER)
  if (base::FeatureList::IsEnabled(brave_player::features::kBravePlayer)) {
    if (params.browser && params.browser->is_type_normal()) {
      auto insert_iter =
          base::ranges::find_if(params.types_enabled, [](const auto& type) {
            return type == PageActionIconType::kSharingHub ||
                   type == brave::kPlaylistPageActionIconType;
          });
      params.types_enabled.insert(insert_iter,
                                  brave::kBravePlayerPageActionIconType);
    }
  }
#endif

  return params;
}

}  // namespace

BravePageActionIconContainerView::BravePageActionIconContainerView(
    PageActionIconParams& params)
    : PageActionIconContainerView(ModifyIconParamsForBrave(params)) {}

BravePageActionIconContainerView::~BravePageActionIconContainerView() = default;

BEGIN_METADATA(BravePageActionIconContainerView, PageActionIconContainerView)
END_METADATA
