/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_BUTTON_H_

#include <chrome/browser/ui/views/toolbar/toolbar_button.h>  // IWYU pragma: export

#include "ui/views/animation/ink_drop_observer.h"
#include "ui/views/metadata/view_factory.h"

class ToolbarButton : public ToolbarButton_ChromiumImpl,
                      public views::InkDropObserver {
  METADATA_HEADER(ToolbarButton, ToolbarButton_ChromiumImpl)

 public:
  using ToolbarButton_ChromiumImpl::ToolbarButton_ChromiumImpl;
  ~ToolbarButton() override;

  // Override icon color for non-disabled button states.
  void SetIconEnabledColorsOverride(std::optional<SkColor> color) {
    icon_enabled_colors_override_ = color;
  }
  bool HasIconEnabledColorsOverride() const {
    return icon_enabled_colors_override_.has_value();
  }

  // ToolbarButton_ChromiumImpl overrides:
  void OnThemeChanged() override;
  void UpdateIcon() override;

  virtual void OnInkDropStateChanged(views::InkDropState state);

 protected:
  // True while the ink drop is ACTIVATED (e.g. an anchored menu or bubble is
  // open). Buttons that build their own images use this to apply
  // kColorToolbarButtonActivated from UpdateIcon().
  bool IsActivated() const { return activated_; }

 private:
  // InkDropObserverImpl overrides:
  void InkDropAnimationStarted() override {}
  void InkDropRippleAnimationEnded(views::InkDropState state) override;

  bool activated_ = false;
  std::optional<SkColor> icon_enabled_colors_override_;
};

BEGIN_VIEW_BUILDER(CHROME_VIEWS_EXPORT,
                   ToolbarButton,
                   ToolbarButton_ChromiumImpl)
END_VIEW_BUILDER

DEFINE_VIEW_BUILDER(CHROME_VIEWS_EXPORT, ToolbarButton)

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_TOOLBAR_BUTTON_H_
