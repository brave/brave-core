/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTROLLER_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTROLLER_IMPL_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/peek/public/peek_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/views/view_observer.h"
#include "ui/views/widget/widget_observer.h"

class BrowserView;
class PeekView;

namespace gfx {
class Rect;
}  // namespace gfx

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace views {
class Widget;
}  // namespace views

// Hosts each Peek in a frameless child widget placed over the browser's
// contents area. A separate widget is needed because views can't paint above
// the native web contents view on every platform.
//
// One Peek is open at a time, over the active tab. It goes when another tab
// is activated.
class PeekControllerImpl : public PeekController,
                           public TabStripModelObserver,
                           public views::ViewObserver,
                           public views::WidgetObserver {
 public:
  PeekControllerImpl(BrowserWindowInterface& browser,
                     BrowserView& browser_view);
  PeekControllerImpl(const PeekControllerImpl&) = delete;
  PeekControllerImpl& operator=(const PeekControllerImpl&) = delete;
  ~PeekControllerImpl() override;

  // The active tab's Peek view and widget.
  PeekView* peek_view_for_testing();
  views::Widget* widget_for_testing();

  // PeekController:
  bool CanOpenFrom(content::WebContents* source) override;
  bool Open(content::WebContents& source,
            const content::OpenURLParams& params) override;
  void Close() override;
  bool IsShowing() const override;
  content::WebContents* GetWebContents() override;

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
  class Peek;

  Peek* GetActivePeek() const;
  Peek* FindPeek(const tabs::TabInterface& source_tab) const;
  Peek* FindPeek(const content::WebContents& peek_contents) const;

  // Destroys `peek` and its page.
  void ClosePeek(Peek& peek);

  gfx::Rect GetTargetBoundsInScreen() const;

  const raw_ref<BrowserWindowInterface> browser_;
  const raw_ref<BrowserView> browser_view_;

  std::vector<std::unique_ptr<Peek>> peeks_;

  base::ScopedObservation<views::View, views::ViewObserver>
      contents_area_observation_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      browser_widget_observation_{this};

  base::WeakPtrFactory<PeekControllerImpl> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTROLLER_IMPL_H_
