// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_

#include <memory>

#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "ui/views/controls/button/label_button.h"

class BraveShieldsActionView : public views::LabelButton {
 public:
  explicit BraveShieldsActionView(Profile* profile);
  BraveShieldsActionView(const BraveShieldsActionView&) = delete;
  BraveShieldsActionView& operator=(const BraveShieldsActionView&) = delete;
  ~BraveShieldsActionView() override;

  void Init();
  void Update();
  // views::LabelButton:
  std::unique_ptr<views::LabelButtonBorder> CreateDefaultBorder()
      const override;

  SkPath GetHighlightPath() const;

 private:
  void ButtonPressed();

  std::unique_ptr<WebUIBubbleManagerT<ShieldsPanelUI>> webui_bubble_manager_;
  Profile* profile_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_
