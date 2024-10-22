/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_SHOW_OPTIONS_EVENT_DETECT_WIDGET_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_SHOW_OPTIONS_EVENT_DETECT_WIDGET_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {
class Widget;
}  // namespace views

namespace sidebar {
class SidebarBrowserTest;
}  // namespace sidebar

class BraveBrowserView;

// Monitors mouse event to show sidebar when mouse is around the left or
// right side of browser window.
// This widget is only used for kShowOnMouseOver option.
class SidebarShowOptionsEventDetectWidget : public views::ViewObserver,
                                            public views::WidgetDelegate {
 public:
  class Delegate {
   public:
    virtual void ShowSidebarControlView() = 0;

   protected:
    virtual ~Delegate() {}
  };

  explicit SidebarShowOptionsEventDetectWidget(BraveBrowserView& browser_view,
                                               Delegate& delegate);
  ~SidebarShowOptionsEventDetectWidget() override;

  SidebarShowOptionsEventDetectWidget(
      const SidebarShowOptionsEventDetectWidget&) = delete;
  SidebarShowOptionsEventDetectWidget& operator=(
      const SidebarShowOptionsEventDetectWidget&) = delete;

  void Show();
  void Hide();
  void SetSidebarOnLeft(bool sidebar_on_left);

  // views::ViewObserver overrides:
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewIsDeleting(views::View* observed_view) override;

 private:
  friend class sidebar::SidebarBrowserTest;

  class ContentsView;

  std::unique_ptr<views::Widget> CreateWidget(Delegate& delegate);
  void AdjustWidgetBounds();

  bool sidebar_on_left_ = true;
  raw_ref<BraveBrowserView> browser_view_;
  raw_ptr<ContentsView, DanglingUntriaged> contents_view_ = nullptr;
  raw_ref<Delegate> delegate_;
  std::unique_ptr<views::Widget> widget_;
  base::ScopedObservation<views::View, views::ViewObserver> observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_SHOW_OPTIONS_EVENT_DETECT_WIDGET_H_
