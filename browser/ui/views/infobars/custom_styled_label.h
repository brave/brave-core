/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_CUSTOM_STYLED_LABEL_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_CUSTOM_STYLED_LABEL_H_

#include <memory>
#include <string>

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/styled_label.h"

// TODO(simonhong): Move common place if needed.
// StyledLabel doesn't allow using custom font for link style.
// Use for setting custom font for link.
class CustomStyledLabel : public views::StyledLabel {
  METADATA_HEADER(CustomStyledLabel, views::StyledLabel)
 public:
  using StyledLabel::StyledLabel;
  CustomStyledLabel(const CustomStyledLabel&) = delete;
  CustomStyledLabel& operator=(const CustomStyledLabel&) = delete;
  ~CustomStyledLabel() override;

 private:
  // views::StyledLabel overrides:
  std::unique_ptr<views::Label> CreateLabel(
      const std::u16string& text,
      const RangeStyleInfo& style_info,
      const gfx::Range& range,
      views::LinkFragment** previous_link_component) const override;

  gfx::Size last_layout_size_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_CUSTOM_STYLED_LABEL_H_
