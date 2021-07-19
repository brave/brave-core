/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_BUTTON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_BUTTON_VIEW_H_

#include "base/timer/timer.h"
#include "ui/views/controls/button/image_button.h"

class SidebarButtonView : public views::ImageButton {
 public:
  static const int kSidebarButtonSize = 42;

  class Delegate {
   public:
    virtual std::u16string GetTooltipTextFor(const views::View* view) const = 0;

   protected:
    virtual ~Delegate() = default;
  };

  explicit SidebarButtonView(Delegate* delegate);
  ~SidebarButtonView() override;

  SidebarButtonView(const SidebarButtonView&) = delete;
  SidebarButtonView operator=(const SidebarButtonView&) = delete;

  // Subclass should return true if want to use mouse long press.
  virtual bool ShouldHandleLongPress() const;
  virtual void OnMouseLongPressed() {}

  // views::ImageButton overrides:
  gfx::Size CalculatePreferredSize() const override;
  std::u16string GetTooltipText(const gfx::Point& p) const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

 private:
  void OnLongPressTimerExipred();
  void StartLongPressTimer();
  void StopLongPressTimer();

  Delegate* delegate_ = nullptr;
  base::OneShotTimer long_press_timer_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_BUTTON_VIEW_H_
