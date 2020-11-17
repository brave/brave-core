/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_window_frame_graphic.h"

#include "base/logging.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"

BraveWindowFrameGraphic::BraveWindowFrameGraphic(Profile* profile)
    : is_tor_window_(brave::IsTorProfile(profile)) {}

BraveWindowFrameGraphic::~BraveWindowFrameGraphic() = default;

void BraveWindowFrameGraphic::Paint(gfx::Canvas* canvas,
                                    const gfx::Rect& frame_bounds) {
  if (!is_tor_window_)
    return;

  constexpr int kGraphicWidth = 360;
  constexpr int kGraphicHeight = 70;
  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  canvas->DrawImageInt(*bundle.GetImageSkiaNamed(IDR_TOR_WINDOW_FRAME_GRAPHIC),
                       0, 0, kGraphicWidth, kGraphicHeight,
                       frame_bounds.width() - kGraphicWidth, 0,
                       kGraphicWidth, kGraphicHeight,
                       false);
}
