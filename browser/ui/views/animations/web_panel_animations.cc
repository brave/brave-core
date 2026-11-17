/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/animations/web_panel_animations.h"

#include "chrome/browser/ui/views/animations/common_animation_values.h"
#include "ui/base/interaction/safe_castable.h"
#include "ui/gfx/animation/tween.h"

DEFINE_SAFE_CAST_TARGET(WebPanelAnimations)

DEFINE_CLASS_BROWSER_ANIMATION_GROUP(WebPanelAnimations, kWebPanel);

DEFINE_CLASS_BROWSER_ANIMATION_MOTION(WebPanelAnimations, kOpen);
DEFINE_CLASS_BROWSER_ANIMATION_MOTION(WebPanelAnimations, kClose);

DEFINE_CLASS_BROWSER_ANIMATION_SEQUENCE(WebPanelAnimations, kPanelWidth);

WebPanelAnimations::WebPanelAnimations() {
  // Persisting the width lets a close reverse from wherever it had got to,
  // instead of snapping back to full width first.
  SetSequenceParams(kWebPanel, Persist(kPanelWidth));
}

WebPanelAnimations::~WebPanelAnimations() = default;

WebPanelAnimations::GroupInfos WebPanelAnimations::GenerateAnimations() const {
  // Duplicated from SidePanelAnimations::GenerateAnimations() so the web panel
  // and the side panel, which share the same space, move identically. Neither
  // value is exported from there.
  constexpr int kDefaultAnimationMs = 350;
  const bool use_flyover =
      browser_animations::GetPanelAnimationType() ==
      browser_animations::PanelAnimationType::kClipAndSingleReflow;
  const gfx::Tween::Type tween = use_flyover
                                     ? gfx::Tween::Type::ACCEL_80_DECEL_20
                                     : gfx::Tween::Type::ACCEL_45_DECEL_88;

  return Groups(
      Group(kWebPanel,
            Motion(kOpen, TotalDurationMs(kDefaultAnimationMs), tween,
                   Animate(kPanelWidth, FromValue(0.0), ToValue(1.0))),
            Motion(kClose, TotalDurationMs(kDefaultAnimationMs), tween,
                   Animate(kPanelWidth, FromValue(1.0), ToValue(0.0)))));
}
