// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_REWARDS_ACTION_STUB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_REWARDS_ACTION_STUB_VIEW_H_

#include <memory>

#include "ui/views/controls/button/label_button.h"
#include "ui/views/view.h"

// A button to take the place of an extension that will be loaded in the future.
// Call SetImage with the BraveActionIconWithBadgeImageSource
// Call highlight etc from ToolbarActionView
class BraveRewardsActionStubView : public views::LabelButton,
                                   public views::ButtonListener {
 public:
  class Delegate {
   public:
    virtual void OnRewardsStubButtonClicked() = 0;
    virtual gfx::Size GetToolbarActionSize() = 0;
   protected:
    ~Delegate() {}
  };

  explicit BraveRewardsActionStubView(Delegate* delegate);
  ~BraveRewardsActionStubView() override;

  // views::ButtonListener
  void ButtonPressed(Button* sender, const ui::Event& event) override;

  // views::LabelButton:
  std::unique_ptr<views::LabelButtonBorder> CreateDefaultBorder()
      const override;
  SkColor GetInkDropBaseColor() const override;
  std::unique_ptr<views::InkDropHighlight> CreateInkDropHighlight()
      const override;

  SkPath GetHighlightPath() const;

 private:
  gfx::Size CalculatePreferredSize() const override;

  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(BraveRewardsActionStubView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_REWARDS_ACTION_STUB_VIEW_H_
