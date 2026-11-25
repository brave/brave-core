/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTROLLER_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTROLLER_IMPL_H_

#include <memory>
#include <optional>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/peek/public/peek_controller.h"
#include "brave/browser/ui/peek/public/peek_shields_panel_provider.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/views/view_observer.h"
#include "ui/views/widget/widget_observer.h"

class BrowserView;
class PeekView;

namespace gfx {
class Rect;
}  // namespace gfx

namespace input {
struct NativeWebKeyboardEvent;
}  // namespace input

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
// Each tab can have one Peek. Only the active tab's is shown; the others are
// hidden, with their pages kept, until their tab is active again.
class PeekControllerImpl : public PeekController,
                           public TabStripModelObserver,
                           public views::ViewObserver,
                           public views::WidgetObserver {
 public:
  PeekControllerImpl(
      BrowserWindowInterface& browser,
      BrowserView& browser_view,
      std::unique_ptr<PeekShieldsPanelProvider> shields_panel_provider);
  PeekControllerImpl(const PeekControllerImpl&) = delete;
  PeekControllerImpl& operator=(const PeekControllerImpl&) = delete;
  ~PeekControllerImpl() override;

  // Moves the active tab's Peek into a tab shown in split view with the active
  // tab, the one the Peek was opened from. Opens a standalone tab instead if
  // the active tab is already in a split.
  void OpenInSplitView();

  // The active tab's Peek view and widget.
  PeekView* peek_view_for_testing();
  views::Widget* widget_for_testing();
  // The contents of `tab`'s Peek, shown or hidden.
  content::WebContents* GetWebContentsForTesting(tabs::TabInterface& tab);
  // Widget activation isn't reliable in browser tests.
  void set_has_focus_for_testing(bool has_focus) {
    has_focus_for_testing_ = has_focus;
  }

  // PeekController:
  bool CanOpenFrom(content::WebContents* source) override;
  bool Open(content::WebContents& source,
            const content::OpenURLParams& params) override;
  void OpenAsTab(content::WebContents& peek_contents) override;
  void ShowCookiesDialogInTab(content::WebContents& peek_contents) override;
  void Close() override;
  bool HandleCommand(int command_id) override;
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

  // Shows the active tab's Peek, if it has one, and hides the others.
  void UpdateVisibility();

  // Destroys `peek` and its page.
  void ClosePeek(Peek& peek);

  // Moves `peek`'s page into a tab after its source tab and destroys `peek`.
  // The new tab is activated if the source tab is active. A hidden Peek is
  // added in the background instead, so its page can't take over the window
  // from the tab the user is on. Returns the moved contents.
  content::WebContents* MoveToTab(Peek& peek);

  // Requests from a Peek's card or page. They're posted by the Peek, since
  // they come from inside event handlers of the objects they destroy.
  void OpenPeekAsTab(Peek& peek);
  void MoveToSplitView(Peek& peek);
  void MoveToTabOrClose(Peek& peek);
  void MoveToTabAndShowShields(Peek& peek);
  void MoveToTabAndShowCookiesDialog(Peek& peek);
  void MoveToTabAndExecuteCommand(int command_id, Peek& peek);
  bool HandleKeyboardEvent(content::WebContents* source,
                           const input::NativeWebKeyboardEvent& event);

  // Whether the user is in the active tab's Peek: its widget is active, or the
  // page is handing a key it didn't handle to the browser.
  bool HasFocus() const;
  gfx::Rect GetTargetBoundsInScreen() const;

  const std::unique_ptr<PeekShieldsPanelProvider> shields_panel_provider_;
  const raw_ref<BrowserWindowInterface> browser_;
  const raw_ref<BrowserView> browser_view_;

  std::vector<std::unique_ptr<Peek>> peeks_;

  bool forwarding_keyboard_event_ = false;
  std::optional<bool> has_focus_for_testing_;
  base::ScopedObservation<views::View, views::ViewObserver>
      contents_area_observation_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      browser_widget_observation_{this};

  base::WeakPtrFactory<PeekControllerImpl> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_CONTROLLER_IMPL_H_
