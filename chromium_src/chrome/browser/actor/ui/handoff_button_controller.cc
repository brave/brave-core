// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Adjust colors and position of button shadow
#define BRAVE_GRADIENT_BUBBLE_FRAME_VIEW_ON_PAINT_SHADOW_BEFORE_DRAW       \
  const SkColor brave_colors[] = {SkColorSetARGB(255, 250, 114, 80),       \
                                  SkColorSetARGB(255, 255, 24, 147),       \
                                  SkColorSetARGB(255, 167, 138, 255)};     \
  color4fs.clear();                                                        \
  for (const auto& color : brave_colors) {                                 \
    color4fs.push_back(SkColor4f::FromColor(color));                       \
  }                                                                        \
  const SkScalar brave_pos[] = {0.0f, 0.5f, 1.0f};                         \
  auto brave_shader = cc::PaintShader::MakeSweepGradient(                  \
      center.x(), center.y(), color4fs.data(), brave_pos, color4fs.size(), \
      SkTileMode::kClamp, 0, 360);                                         \
  shadow_flags.setShader(std::move(brave_shader));

// Tighten the button's content padding. Brave's location bar centers its URL
// text, so the centered handoff button covers it more than in upstream where
// the URL is left-aligned; a smaller button is less obtrusive.
#define BRAVE_HANDOFF_BUTTON_CONTENT_PADDING(default_value) \
  gfx::Insets::TLBR(4, 6, 4, 10)

// Nudge the button down when the location bar is visible, so the visible
// button sits lower and covers less of Brave's centered URL text.
#define BRAVE_HANDOFF_BUTTON_Y_TAB_STRIP_VISIBLE_ADJUST \
  +(kHandoffButtonPreferredHeight / 3)

#include <chrome/browser/actor/ui/handoff_button_controller.cc>
#undef BRAVE_HANDOFF_BUTTON_Y_TAB_STRIP_VISIBLE_ADJUST
#undef BRAVE_HANDOFF_BUTTON_CONTENT_PADDING
#undef BRAVE_GRADIENT_BUBBLE_FRAME_VIEW_ON_PAINT_SHADOW_BEFORE_DRAW
