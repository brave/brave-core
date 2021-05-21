/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_SHOW_OPTIONS_EVENT_DETECT_WIDGET_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_SHOW_OPTIONS_EVENT_DETECT_WIDGET_H_

#include <memory>

#include "base/scoped_observation.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {
class Widget;
}  // namespace views

class BraveBrowserView;

class SidebarShowOptionsEventDetectWidget : public views::ViewObserver,
                                            public views::WidgetDelegate {
 public:
  class Delegate {
   public:
    virtual void ShowSidebar() = 0;
    virtual bool ShouldShowOnHover() = 0;

   protected:
    virtual ~Delegate() {}
  };

  explicit SidebarShowOptionsEventDetectWidget(BraveBrowserView* browser_view,
                                               Delegate* delegate);
  ~SidebarShowOptionsEventDetectWidget() override;

  SidebarShowOptionsEventDetectWidget(
      const SidebarShowOptionsEventDetectWidget&) = delete;
  SidebarShowOptionsEventDetectWidget& operator=(
      const SidebarShowOptionsEventDetectWidget&) = delete;

  void Show();
  void Hide();

  // views::ViewObserver overrides:
  void OnViewBoundsChanged(views::View* observed_view) override;

 private:
  class ContentsView;

  std::unique_ptr<views::Widget> CreateWidget(Delegate* delegate);
  void AdjustWidgetBounds();

  BraveBrowserView* browser_view_ = nullptr;
  ContentsView* contents_view_ = nullptr;
  Delegate* delegate_ = nullptr;
  std::unique_ptr<views::Widget> widget_;
  base::ScopedObservation<views::View, views::ViewObserver> observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_SHOW_OPTIONS_EVENT_DETECT_WIDGET_H_
