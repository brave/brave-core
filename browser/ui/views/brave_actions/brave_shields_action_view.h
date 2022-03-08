// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_

#include <memory>

#include "brave/browser/ui/brave_shields_data_controller.h"
#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "ui/views/controls/button/label_button.h"

class TabStripModel;
class IconWithBadgeImageSource;

class BraveShieldsActionView
    : public views::LabelButton,
      public brave_shields::BraveShieldsDataController::Observer,
      public TabStripModelObserver {
 public:
  explicit BraveShieldsActionView(Profile* profile,
                                  TabStripModel* tab_strip_model);
  BraveShieldsActionView(const BraveShieldsActionView&) = delete;
  BraveShieldsActionView& operator=(const BraveShieldsActionView&) = delete;
  ~BraveShieldsActionView() override;

  void Init();
  void Update();
  // views::LabelButton:
  std::unique_ptr<views::LabelButtonBorder> CreateDefaultBorder()
      const override;
  std::u16string GetTooltipText(const gfx::Point& p) const override;
  SkPath GetHighlightPath() const;

 private:
  void ButtonPressed();
  bool SchemeIsLocal(GURL url);
  void UpdateIconState();
  gfx::ImageSkia GetIconImage(bool is_enabled);
  std::unique_ptr<IconWithBadgeImageSource> GetImageSource();
  // brave_shields::BraveShieldsDataController
  void OnResourcesChanged() override;
  // TabStripModelObserver
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  std::unique_ptr<WebUIBubbleManagerT<ShieldsPanelUI>> webui_bubble_manager_;
  Profile* profile_ = nullptr;
  TabStripModel* tab_strip_model_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_
