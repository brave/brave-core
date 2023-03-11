// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_STYLED_LABEL_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_STYLED_LABEL_H_

class CustomStyledLabel;

#define CreateLabel                                                        \
  CreateLabel_UnUsed(const std::u16string& text,                           \
                     const RangeStyleInfo& style_info,                     \
                     const gfx::Range& range,                              \
                     views::LinkFragment** previous_link_component) const; \
  friend class ::CustomStyledLabel;                                        \
  virtual std::unique_ptr<views::Label> CreateLabel

#include "src/ui/views/controls/styled_label.h"  // IWYU pragma: export

#undef CreateLabel

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_STYLED_LABEL_H_
