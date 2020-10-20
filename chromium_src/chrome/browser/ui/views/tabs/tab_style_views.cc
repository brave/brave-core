/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_GM2_TAB_STYLE_H                                                 \
 private:                                                                     \
  gfx::FontList semibold_font_;

#define BRAVE_GM2_TAB_STYLE                                                   \
  semibold_font_ = normal_font_.DeriveWithWeight(gfx::Font::Weight::SEMIBOLD);

#define BRAVE_CALCULATE_COLORS                                                \
    const SkColor inactive_non_hovered_fg_color =                             \
        SkColorSetA(foreground_color,                                         \
                    gfx::Tween::IntValueBetween(0.7,                          \
                                                SK_AlphaTRANSPARENT,          \
                                                SK_AlphaOPAQUE));             \
    const SkColor final_fg_color =                                            \
        (tab_->IsActive() || tab_->mouse_hovered()) ?                         \
            foreground_color : inactive_non_hovered_fg_color;                 \
    return { final_fg_color, background_color};

#define BRAVE_GET_FONT_LIST                                                   \
  return tab_->IsActive() ? semibold_font_ : normal_font_;

#include "../../../../../../../chrome/browser/ui/views/tabs/tab_style_views.cc"

#undef BRAVE_GM2_TAB_STYLE_H
#undef BRAVE_GM2_TAB_STYLE
#undef BRAVE_CALCULATE_COLORS
#undef BRAVE_GET_FONT_LIST
