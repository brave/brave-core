/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/top_container_background.h"

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

TopContainerBackground::TopContainerBackground()
    : shadow_(this, /*corner_radius=*/0, kShadow) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(true);
  SetCanProcessEventsWithinSubtree(false);
}

TopContainerBackground::~TopContainerBackground() = default;

BEGIN_METADATA(TopContainerBackground)
END_METADATA
