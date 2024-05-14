/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/custom_styled_label.h"

#include <utility>

#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/link_fragment.h"

CustomStyledLabel::~CustomStyledLabel() = default;

std::unique_ptr<views::Label> CustomStyledLabel::CreateLabel(
    const std::u16string& text,
    const RangeStyleInfo& style_info,
    const gfx::Range& range,
    views::LinkFragment** previous_link_fragment) const {
  if (style_info.text_style != views::style::STYLE_LINK)
    return StyledLabel::CreateLabel(text, style_info, range,
                                    previous_link_fragment);

  std::unique_ptr<views::Label> result;
  auto link = std::make_unique<views::LinkFragment>(
      text, text_context_, views::style::STYLE_LINK, *previous_link_fragment);
  *previous_link_fragment = link.get();
  link->SetCallback(style_info.callback);
  link->SetFontList(style_info.custom_font.value());
  if (!style_info.accessible_name.empty())
    link->SetAccessibleName(style_info.accessible_name);

  result = std::move(link);

  if (style_info.override_color)
    result->SetEnabledColor(style_info.override_color.value());
  if (!style_info.tooltip.empty())
    result->SetTooltipText(style_info.tooltip);
  if (!style_info.accessible_name.empty())
    result->SetAccessibleName(style_info.accessible_name);
  if (auto* colour = absl::get_if<SkColor>(&displayed_on_background_color_)) {
    result->SetBackgroundColor(*colour);
  } else if (auto* colour_id =
                 absl::get_if<ui::ColorId>(&displayed_on_background_color_)) {
    result->SetBackgroundColor(*colour_id);
  }

  result->SetAutoColorReadabilityEnabled(auto_color_readability_enabled_);
  result->SetSubpixelRenderingEnabled(subpixel_rendering_enabled_);
  return result;
}

BEGIN_METADATA(CustomStyledLabel)
END_METADATA
