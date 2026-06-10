/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/layout/brave_browser_view_tabbed_layout_impl.h"

#include <chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.cc>

// `BrowserViewTabbedLayoutImpl::SeparatorInfo` is a private nested struct
// defined inside the upstream .cc above. Defining the override here keeps the
// struct out of any header while still letting Brave consolidate the old
// `GetTopSeparatorType()` / `ShadowOverlayVisible()` tweaks into a single
// hook.
BrowserViewTabbedLayoutImpl::SeparatorInfo
BraveBrowserViewTabbedLayoutImpl::CalculateSeparatorInfo() const {
  // Suppress every top separator when there is no visible top UI (toolbar and
  // bookmark bar). This fixes a 1px visible separator at the top of the
  // contents view when in browser fullscreen, where the top chrome is hidden.
  if (!delegate().IsToolbarVisible() && !delegate().IsBookmarkBarVisible()) {
    return SeparatorInfo();
  }

  SeparatorInfo info = BrowserViewTabbedLayoutImpl::CalculateSeparatorInfo();

  // Brave manages its own rounded-corners shadow around the contents and side
  // panel via BraveContentsViewUtil. Suppress the upstream shadow overlay (and
  // its accompanying main-area padding) so it doesn't double up.
  info.shadow_box = false;

  // When the upstream choice is "no top-container separator" (either nothing
  // at all, or only a multi-contents separator), Brave promotes that to a
  // full-width top-container separator unless the rounded-corners contents
  // view is in use - in which case the rounded shape provides the visual
  // divider and no separator is wanted.
  if (!info.top_container_separator) {
    const bool rounded =
        delegate().ShouldUseBraveWebViewRoundedCornersForContents();
    info.top_container_separator = !rounded;
    info.multi_contents_separator = false;
  }

  return info;
}

bool BraveBrowserViewTabbedLayoutImpl::GetTopContainerSeparatorForTesting()
    const {
  return CalculateSeparatorInfo().top_container_separator;
}

bool BraveBrowserViewTabbedLayoutImpl::GetMultiContentsSeparatorForTesting()
    const {
  return CalculateSeparatorInfo().multi_contents_separator;
}

bool BraveBrowserViewTabbedLayoutImpl::GetShadowBoxForTesting() const {
  return CalculateSeparatorInfo().shadow_box;
}
