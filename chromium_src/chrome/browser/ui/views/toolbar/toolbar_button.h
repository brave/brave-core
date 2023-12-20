/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_BUTTON_H_

#include "ui/views/animation/ink_drop_observer.h"

#define ToolbarButton ToolbarButton_ChromiumImpl
#define SetHighlight                                  \
  SetMenuModel(std::unique_ptr<ui::MenuModel> model); \
  bool HasVectorIcons() const;                        \
  const gfx::VectorIcon& GetVectorIcon() const;       \
  const gfx::VectorIcon& GetVectorTouchIcon() const;  \
  virtual void SetHighlight

#define kDefaultIconSize \
  kDefaultIconSize = 18; \
  static constexpr int kDefaultIconSize_UnUsed
#include "src/chrome/browser/ui/views/toolbar/toolbar_button.h"  // IWYU pragma: export
#undef kDefaultIconSize
#undef SetHighlight
#undef ToolbarButton

class ToolbarButton : public ToolbarButton_ChromiumImpl,
                      public views::InkDropObserver {
  METADATA_HEADER(ToolbarButton, ToolbarButton_ChromiumImpl)

 public:
  using ToolbarButton_ChromiumImpl::ToolbarButton_ChromiumImpl;
  ~ToolbarButton() override;

  // ToolbarButton_ChromiumImpl overrides:
  void OnThemeChanged() override;

  virtual void OnInkDropStateChanged(views::InkDropState state);

 private:
  // InkDropObserverImpl overrides:
  void InkDropAnimationStarted() override {}
  void InkDropRippleAnimationEnded(views::InkDropState state) override;

  bool activated_ = false;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_BUTTON_H_
