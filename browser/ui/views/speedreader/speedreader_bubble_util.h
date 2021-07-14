/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_BUBBLE_UTIL_H_
#define BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_BUBBLE_UTIL_H_

#include <memory>

#include "ui/gfx/font.h"
#include "ui/views/controls/link.h"

namespace gfx {
class FontList;
}  // namespace gfx

namespace views {
class StyledLabel;
}  // namespace views

namespace speedreader {

// Line height for multiline labels.
constexpr int kLineHeight = 16;

// Spacing between child nodes in box layout
constexpr int kBoxLayoutChildSpacing = 10;

// Separator used in StyledLabel
constexpr char16_t kSpeedreaderSeparator[] = u" ";

// Reader Mode blurple color
constexpr SkColor kColorReaderBlurple = SkColorSetRGB(0x4c, 0x54, 0xd2);

// Get fonts for Speedreader views
gfx::FontList GetFont(int font_size,
                      gfx::Font::Weight weight = gfx::Font::Weight::NORMAL);

// Create a StyledLabel that ends with a link.
std::unique_ptr<views::StyledLabel> BuildLabelWithEndingLink(
    const std::u16string& reg_text,
    const std::u16string& link_text,
    views::Link::ClickedCallback callback);

}  // namespace speedreader

#endif  // BRAVE_BROWSER_UI_VIEWS_SPEEDREADER_SPEEDREADER_BUBBLE_UTIL_H_
