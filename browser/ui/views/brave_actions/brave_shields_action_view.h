// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/menu_button_controller.h"
#include "ui/views/widget/widget.h"

class TabStripModel;
class IconWithBadgeImageSource;

class BraveShieldsActionView
    : public views::LabelButton,
      public brave_shields::BraveShieldsTabHelper::Observer,
      public TabStripModelObserver {
  METADATA_HEADER(BraveShieldsActionView, views::LabelButton)
 public:
  DECLARE_CLASS_ELEMENT_IDENTIFIER_VALUE(kShieldsActionIcon);
  explicit BraveShieldsActionView(
      BrowserWindowInterface* browser_window_interface);
  BraveShieldsActionView(const BraveShieldsActionView&) = delete;
  BraveShieldsActionView& operator=(const BraveShieldsActionView&) = delete;
  ~BraveShieldsActionView() override;

  void Init();
  void Update();

  // views::LabelButton:
  std::unique_ptr<views::LabelButtonBorder> CreateDefaultBorder()
      const override;
  std::u16string GetTooltipText(const gfx::Point& p) const override;
  void OnThemeChanged() override;

  SkPath GetHighlightPath() const;
  views::Widget* GetBubbleWidget() {
    return webui_bubble_manager_->GetBubbleWidget();
  }

 private:
  void ButtonPressed(BrowserWindowInterface* browser_window_interface);
  bool IsPageInReaderMode(content::WebContents* web_contents);
  bool ShouldShowBubble(content::WebContents* web_contents);
  void UpdateIconState();
  gfx::ImageSkia GetIconImage(bool is_enabled);
  std::unique_ptr<IconWithBadgeImageSource> GetImageSource();

  // brave_shields::BraveShieldsTabHelper
  void OnResourcesChanged() override;
  void OnShieldsEnabledChanged() override;

  // TabStripModelObserver
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  raw_ptr<views::MenuButtonController> menu_button_controller_ = nullptr;
  raw_ref<Profile> profile_;
  raw_ref<TabStripModel> tab_strip_model_;
  std::unique_ptr<WebUIBubbleManager> webui_bubble_manager_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_
