/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTROLLER_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTROLLER_IMPL_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/peek/public/peek_controller.h"
#include "brave/browser/ui/peek/public/peek_shields_panel_provider.h"
#include "brave/browser/ui/views/peek/peek_contents.h"
#include "brave/browser/ui/views/peek/peek_view.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/views/view_observer.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

class BrowserView;

// Hosts Peek in a frameless child widget placed over the browser's contents
// area. A separate widget is needed because views can't paint above the
// native web contents view on every platform.
class PeekControllerImpl : public PeekController,
                           public PeekContents::Delegate,
                           public PeekView::Delegate,
                           public TabStripModelObserver,
                           public views::ViewObserver,
                           public views::WidgetObserver {
 public:
  PeekControllerImpl(BrowserWindowInterface& browser,
                     BrowserView& browser_view);
  PeekControllerImpl(const PeekControllerImpl&) = delete;
  PeekControllerImpl& operator=(const PeekControllerImpl&) = delete;
  ~PeekControllerImpl() override;

  PeekView* peek_view_for_testing() { return peek_view_; }
  views::Widget* widget_for_testing() { return widget_.get(); }

  // PeekController:
  void SetShieldsPanelProvider(
      std::unique_ptr<PeekShieldsPanelProvider> provider) override;
  bool CanOpenFrom(content::WebContents* source) override;
  bool Open(content::WebContents& source,
            const content::OpenURLParams& params) override;
  void OpenAsTab() override;
  void Close() override;
  bool IsShowing() const override;
  content::WebContents* GetWebContents() override;

  // PeekContents::Delegate:
  void OnPeekCloseRequested() override;
  void OnPeekNavigationStateChanged() override;
  void OnPeekMoveToTabRequired() override;
  void OnPeekMoveToTabOrCloseRequested() override;
  bool OnPeekTakeFocus(bool reverse) override;

  // PeekView::Delegate:
  void OnPeekViewCloseRequested() override;
  void OnPeekViewOpenAsTabRequested() override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // views::ViewObserver:
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewIsDeleting(views::View* observed_view) override;

  // views::WidgetObserver:
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

 private:
  void Show(std::unique_ptr<content::WebContents> contents);
  void OpenAsTabOrClose();
  void CreateWidget();
  void UpdateWidgetBounds();
  gfx::Rect GetTargetBoundsInScreen() const;

  // Close and open-as-tab requests come from inside event handlers of the
  // widget or the WebContents being torn down, so they run asynchronously.
  void PostTask(void (PeekControllerImpl::*method)());

  void OnWidgetCloseRequested(views::Widget::ClosedReason reason);

  // `contents_` must outlive `widget_`, which shows it, and the provider is
  // used by the widget's views.
  std::unique_ptr<PeekShieldsPanelProvider> shields_panel_provider_;
  std::unique_ptr<PeekContents> contents_;
  std::unique_ptr<views::Widget> widget_;
  base::ScopedObservation<views::View, views::ViewObserver>
      contents_area_observation_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      browser_widget_observation_{this};

  const raw_ref<BrowserWindowInterface> browser_;
  const raw_ref<BrowserView> browser_view_;
  raw_ptr<PeekView> peek_view_ = nullptr;

  base::WeakPtrFactory<PeekControllerImpl> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTROLLER_IMPL_H_
