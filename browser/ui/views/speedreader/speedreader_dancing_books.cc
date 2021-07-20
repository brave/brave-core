/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/speedreader/speedreader_dancing_books.h"

#include "brave/app/vector_icons/vector_icons.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"

namespace {

constexpr SkColor kBraveSpeedreaderGraphicLight =
    SkColorSetRGB(0xE9, 0xEC, 0xEF);
constexpr SkColor kBraveSpeedreaderGraphicDark =
    SkColorSetRGB(0x49, 0x50, 0x57);

constexpr int kMinimumWidth = 287;
constexpr int kMinimumHeight = 61;

}  // anonymous namespace

namespace speedreader {

SpeedreaderDancingBooks::SpeedreaderDancingBooks() : views::View() {
  graphics_locations_[0] =
      std::make_pair(gfx::Vector2d(0, 35), &kBraveSpeedreaderGraphicLinesIcon);
  graphics_locations_[1] =
      std::make_pair(gfx::Vector2d(29, 18), &kBraveSpeedreaderGraphicBook1Icon);
  graphics_locations_[2] =
      std::make_pair(gfx::Vector2d(91, 28), &kBraveSpeedreaderGraphicBook2Icon);
  graphics_locations_[3] = std::make_pair(gfx::Vector2d(159, 11),
                                          &kBraveSpeedreaderGraphicBook2Icon);
  graphics_locations_[4] = std::make_pair(gfx::Vector2d(204, 24),
                                          &kBraveSpeedreaderGraphicLinesIcon);
  graphics_locations_[5] =
      std::make_pair(gfx::Vector2d(233, 0), &kBraveSpeedreaderGraphicBook3Icon);
}

SpeedreaderDancingBooks::~SpeedreaderDancingBooks() {
  // We need a complex destructor, but no deallocation is necessary since
  // graphics_locations_ only contains weak pointers.
}

void SpeedreaderDancingBooks::OnPaint(gfx::Canvas* canvas) {
  SkColor color =
      ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors()
          ? kBraveSpeedreaderGraphicDark
          : kBraveSpeedreaderGraphicLight;

  int xclip = 0;
  int clip_width = GetBoundsInScreen().width();
  if (clip_width < kMinimumWidth) {
    // clip left
    xclip = clip_width - kMinimumWidth;
  } else if (clip_width > kMinimumWidth) {
    // center
    xclip = (clip_width - kMinimumWidth) / 2;
  }

  for (const BookGraphic& graphic : graphics_locations_) {
    canvas->Save();
    gfx::Vector2d translate = graphic.first;
    translate.set_x(translate.x() + xclip);
    canvas->Translate(translate);
    gfx::PaintVectorIcon(canvas, *graphic.second, color);
    canvas->Restore();
  }

  views::View::OnPaint(canvas);
}

gfx::Size SpeedreaderDancingBooks::CalculatePreferredSize() const {
  return GetMinimumSize();
}

gfx::Size SpeedreaderDancingBooks::GetMinimumSize() const {
  return gfx::Size(kMinimumWidth, kMinimumHeight);
}

}  // namespace speedreader
