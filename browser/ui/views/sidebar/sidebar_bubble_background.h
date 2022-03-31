/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_BUBBLE_BACKGROUND_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_BUBBLE_BACKGROUND_H_

#include "base/memory/raw_ptr.h"
#include "ui/views/background.h"

namespace views {
class BubbleBorder;
}  // namespace views

class BubbleBorderWithArrow;

class SidebarBubbleBackground : public views::Background {
 public:
  explicit SidebarBubbleBackground(BubbleBorderWithArrow* border);
  ~SidebarBubbleBackground() override;

  SidebarBubbleBackground(const SidebarBubbleBackground&) = delete;
  SidebarBubbleBackground& operator=(const SidebarBubbleBackground) = delete;

  // views::Background overrides:
  void Paint(gfx::Canvas* canvas, views::View* view) const override;

 private:
  raw_ptr<BubbleBorderWithArrow> border_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_BUBBLE_BACKGROUND_H_
