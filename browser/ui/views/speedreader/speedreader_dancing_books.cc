/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/speedreader/speedreader_dancing_books.h"

#include "brave/app/vector_icons/vector_icons.h"
#include "include/core/SkColor.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/native_theme/native_theme.h"

namespace {
constexpr SkColor kBraveSpeedreaderGraphicLight =
    SkColorSetRGB(0xE9, 0xEC, 0xEF);
constexpr SkColor kBraveSpeedreaderGraphicDark =
    SkColorSetRGB(0x49, 0x50, 0x57);
}  // anonymous namespace

namespace speedreader {

void SpeedreaderDancingBooks::OnPaint(gfx::Canvas* canvas) {
  SkColor color =
      ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors()
          ? kBraveSpeedreaderGraphicDark
          : kBraveSpeedreaderGraphicLight;

  canvas->Save();
  canvas->Translate(gfx::Vector2d(0, 35));
  gfx::PaintVectorIcon(canvas, kBraveSpeedreaderGraphicLinesIcon, color);
  canvas->Restore();

  canvas->Save();
  canvas->Translate(gfx::Vector2d(29, 18));
  gfx::PaintVectorIcon(canvas, kBraveSpeedreaderGraphicBook1Icon, color);
  canvas->Restore();

  canvas->Save();
  canvas->Translate(gfx::Vector2d(91, 28));
  gfx::PaintVectorIcon(canvas, kBraveSpeedreaderGraphicBook2Icon, color);
  canvas->Restore();

  canvas->Save();
  canvas->Translate(gfx::Vector2d(159, 11));
  gfx::PaintVectorIcon(canvas, kBraveSpeedreaderGraphicBook2Icon, color);
  canvas->Restore();

  canvas->Save();
  canvas->Translate(gfx::Vector2d(204, 24));
  gfx::PaintVectorIcon(canvas, kBraveSpeedreaderGraphicLinesIcon, color);
  canvas->Restore();

  canvas->Save();
  canvas->Translate(gfx::Vector2d(233, 0));
  gfx::PaintVectorIcon(canvas, kBraveSpeedreaderGraphicBook3Icon, color);
  canvas->Restore();

  views::View::OnPaint(canvas);
}

gfx::Size SpeedreaderDancingBooks::CalculatePreferredSize() const {
  return GetMinimumSize();
}

gfx::Size SpeedreaderDancingBooks::GetMinimumSize() const {
  return gfx::Size(287, 61);
}

}  // namespace speedreader
