// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_ACCENT_COLOR_BRAVE_TAB_ACCENT_COLOR_PALETTE_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_ACCENT_COLOR_BRAVE_TAB_ACCENT_COLOR_PALETTE_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/views/tabs/accent_color/brave_tab_accent_types.h"
#include "ui/color/color_provider.h"

namespace accent_color {

struct TabAccentColorsParams {
  SkColor container_color = 0;
  bool is_dark = false;
  bool is_pinned = false;

  enum class State {
    kActive = 0,
    kHovered,
    kInactive,
    kNum,
  };
  State state = State::kInactive;

  auto operator<=>(const TabAccentColorsParams&) const = default;
};

// Returns a set of accent colors (border, background, icon) for a container tab
// given the container background color and the tab's visual state.
TabAccentColors GetTabAccentColors(const TabAccentColorsParams& params,
                                   const ui::ColorProvider* color_provider);

}  // namespace accent_color

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_ACCENT_COLOR_BRAVE_TAB_ACCENT_COLOR_PALETTE_H_
