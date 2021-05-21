/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_actions/brave_action_icon_with_badge_image_source.h"

#include <algorithm>

#include "base/strings/utf_string_conversions.h"
#include "cc/paint/paint_flags.h"
#include "chrome/grit/theme_resources.h"
#include "extensions/common/constants.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/skia_paint_util.h"

absl::optional<int>
BraveActionIconWithBadgeImageSource::GetCustomGraphicSize() {
  return kBraveActionGraphicSize;
}

absl::optional<int>
BraveActionIconWithBadgeImageSource::GetCustomGraphicXOffset() {
  return std::floor(
      (size().width() - kBraveActionRightMargin - kBraveActionGraphicSize) /
      2.0);
}

absl::optional<int>
BraveActionIconWithBadgeImageSource::GetCustomGraphicYOffset() {
  return std::floor((size().height() - kBraveActionGraphicSize) / 2.0);
}

void BraveActionIconWithBadgeImageSource::PaintBadge(gfx::Canvas* canvas) {
    if (!badge_ || badge_->text.empty())
    return;

  SkColor text_color = SkColorGetA(badge_->text_color) == SK_AlphaTRANSPARENT
                           ? SK_ColorWHITE
                           : badge_->text_color;

  SkColor background_color =
      SkColorSetA(badge_->background_color, SK_AlphaOPAQUE);

  // Always use same height to avoid jumping up and down with different
  // characters which will differ slightly,
  // but vary the width so we cover as little of the icon as possible.
  constexpr int kBadgeHeight = 12;
  constexpr int kBadgeMaxWidth = 14;
  constexpr int kVPadding = 1;
  constexpr int kVMarginTop = 2;
  const int kTextHeightTarget = kBadgeHeight - (kVPadding * 2);
  int h_padding = 2;
  int text_max_width = kBadgeMaxWidth - (h_padding * 2);

  ui::ResourceBundle* rb = &ui::ResourceBundle::GetSharedInstance();
  gfx::FontList base_font = rb->GetFontList(ui::ResourceBundle::BaseFont)
                                .DeriveWithHeightUpperBound(kTextHeightTarget);
  std::u16string utf16_text = base::UTF8ToUTF16(badge_->text);

  // Calculate best font size to fit maximum Width and constant Height
  int text_height = 0;
  int text_width = 0;
  gfx::Canvas::SizeStringInt(utf16_text, base_font, &text_width,
                             &text_height,
                             0, gfx::Canvas::NO_ELLIPSIS);
  // Leaving extremely verbose log lines commented in case we want to change
  // any sizes in this algorithm, these logs are helpful.
  // LOG(ERROR) << "BraveAction badge text size initial, "
  //            << "w:"
  //            << text_width
  //            << " h:"
  //            << text_height;
  if (text_width > text_max_width) {
    // Too wide
    // Reduce the padding
    h_padding -= 1;
    text_max_width += 2;  // 2 * padding delta
    // If still cannot squeeze it in, reduce font size
    if (text_width > text_max_width) {
      // Reduce font size until we find the first one that fits within the width
      // TODO(petermill): Consider adding minimum font-size and adjusting
      // |max_decrement_attempts| accordingly
      int max_decrement_attempts = base_font.GetFontSize() - 1;
      for (int i = 0; i < max_decrement_attempts; ++i) {
        base_font =
            base_font.Derive(-1, 0, gfx::Font::Weight::NORMAL);
        gfx::Canvas::SizeStringInt(utf16_text, base_font, &text_width,
                                   &text_height, 0, gfx::Canvas::NO_ELLIPSIS);
        // LOG(ERROR) << "reducing to font size - w:" << text_width << " h:" <<
        // text_height;
        if (text_width <= text_max_width)
          break;
      }
    }
  } else if (text_height < kTextHeightTarget) {
    // Narrow enough, but could grow taller
    // Increase font size until text fills height and is not too wide
    // LOG(ERROR) << "can increase height";
    constexpr int kMaxIncrementAttempts = 5;
    for (size_t i = 0; i < kMaxIncrementAttempts; ++i) {
      int w = 0;
      int h = 0;
      gfx::FontList bigger_font =
          base_font.Derive(1, 0, gfx::Font::Weight::NORMAL);
      gfx::Canvas::SizeStringInt(utf16_text, bigger_font, &w, &h, 0,
                                gfx::Canvas::NO_ELLIPSIS);
      if (h > kTextHeightTarget || w > text_max_width)
        break;
      base_font = bigger_font;
      text_width = w;
      text_height = h;
      // LOG(ERROR) << "increasing to font size - w:"
      //            << text_width
      //            << " h:" << text_height;
    }
  }

  // Calculate badge size. It is clamped to a min width just because it looks
  // silly if it is too skinny.
  int badge_width = text_width + h_padding * 2;
  // Has to at least be as wide as it is tall, otherwise it looks weird
  badge_width = std::max(kBadgeHeight, badge_width);

  const gfx::Rect icon_area = GetIconAreaRect();
  // Force the pixel width of badge to be either odd (if the icon width is odd)
  // or even otherwise. If there is a mismatch you get http://crbug.com/26400.
  if (icon_area.width() != 0 && (badge_width % 2 != icon_area.width() % 2))
    badge_width += 1;

  // Calculate the badge background rect. It is anchored to a specific position
  const int badge_offset_x = icon_area.width() - kBadgeMaxWidth;
  const int badge_offset_y = kVMarginTop;
  gfx::Rect rect(icon_area.x() + badge_offset_x, icon_area.y() + badge_offset_y,
                 badge_width, kBadgeHeight);
  cc::PaintFlags rect_flags;
  rect_flags.setStyle(cc::PaintFlags::kFill_Style);
  rect_flags.setAntiAlias(true);
  rect_flags.setColor(background_color);

  // Paint the backdrop.
  constexpr int kOuterCornerRadius = 5;
  canvas->DrawRoundRect(rect, kOuterCornerRadius, rect_flags);

  // Paint the text.
  const int kTextExtraVerticalPadding = (kTextHeightTarget - text_height) / 2;
  const int kVerticalPadding = kVPadding + kTextExtraVerticalPadding;
  // l, t, r, b
  rect.Inset(0, kVerticalPadding, 0, kVerticalPadding);
  // Draw string with ellipsis if it does not fit
  canvas->DrawStringRectWithFlags(utf16_text, base_font, text_color, rect,
                                  gfx::Canvas::TEXT_ALIGN_CENTER);
}

gfx::Rect BraveActionIconWithBadgeImageSource::GetIconAreaRect() const {
  return gfx::Rect(size());
}
