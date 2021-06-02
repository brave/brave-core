/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/speedreader/speedreader_bubble_util.h"

#include <string>
#include <vector>

#include "ui/views/controls/styled_label.h"

namespace speedreader {

constexpr int kFontSizeLabel = 12;

gfx::FontList GetFont(int font_size, gfx::Font::Weight weight) {
  auto font = gfx::FontList();
  return font.DeriveWithSizeDelta(std::abs(font_size - font.GetFontSize()))
      .DeriveWithWeight(weight);
}

// Create a StyledLabel that ends with a link.
std::unique_ptr<views::StyledLabel> BuildLabelWithEndingLink(
    const std::u16string& reg_text,
    const std::u16string& link_text,
    views::Link::ClickedCallback callback) {
  auto label = std::make_unique<views::StyledLabel>();
  std::u16string text = reg_text;
  text.append(base::ASCIIToUTF16(kSpeedreaderSeparator));
  size_t default_format_offset = text.length();
  text.append(link_text);
  label->SetText(text);

  // Setup styles
  views::StyledLabel::RangeStyleInfo style_link =
      views::StyledLabel::RangeStyleInfo::CreateForLink(callback);
  style_link.override_color = kColorReaderBlurple;

  views::StyledLabel::RangeStyleInfo style_default;
  style_default.custom_font = GetFont(kFontSizeLabel);

  // Apply styles
  label->AddStyleRange(gfx::Range(0, default_format_offset), style_default);
  label->AddStyleRange(gfx::Range(default_format_offset, text.length()),
                       style_link);
  return label;
}

}  // namespace speedreader
