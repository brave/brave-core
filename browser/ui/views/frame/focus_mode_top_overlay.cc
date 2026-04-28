/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/focus_mode_top_overlay.h"

#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"

namespace {

constexpr ViewShadow::ShadowParameters kShadow{
    .offset_x = 0,
    .offset_y = 2,
    .blur_radius = 8,
    .shadow_color = SkColorSetA(SK_ColorBLACK, 0.15 * 255)};

}  // namespace

FocusModeTopOverlay::FocusModeTopOverlay()
    : shadow_(this, gfx::RoundedCornersF(0), kShadow) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(true);
}

FocusModeTopOverlay::~FocusModeTopOverlay() = default;

BEGIN_METADATA(FocusModeTopOverlay)
END_METADATA
