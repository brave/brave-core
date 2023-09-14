/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_ADD_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_ADD_BUTTON_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

class BraveBrowser;

class SidebarItemAddButton : public SidebarButtonView,
                             public views::WidgetObserver {
 public:
  METADATA_HEADER(SidebarItemAddButton);
  explicit SidebarItemAddButton(BraveBrowser* browser,
                                const std::u16string& accessible_name);
  ~SidebarItemAddButton() override;

  SidebarItemAddButton(const SidebarItemAddButton&) = delete;
  SidebarItemAddButton& operator=(const SidebarItemAddButton&) = delete;

  // views::WidgetObserver overrides:
  void OnWidgetDestroying(views::Widget* widget) override;

  bool IsBubbleVisible() const;

 private:
  void ShowBubble();

  void UpdateButtonImages();
  void OnButtonPressed();

  raw_ptr<BraveBrowser> browser_ = nullptr;
  base::CallbackListSubscription on_enabled_changed_subscription_;
  base::ScopedObservation<views::Widget, views::WidgetObserver> observation_{
      this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_ADD_BUTTON_H_
