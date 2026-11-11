/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_CONTROLLER_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_CONTROLLER_H_

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class BrowserView;
class BraveMultiContentsView;

namespace content {
class WebContents;
}  // namespace content

namespace sidebar {

class SidebarWebPanelController : public TabStripModelObserver {
 public:
  // |web_panel_state_changed| runs whenever the open panel changes.
  SidebarWebPanelController(BrowserView& browser,
                            base::RepeatingClosure web_panel_state_changed);
  ~SidebarWebPanelController() override;

  SidebarWebPanelController(const SidebarWebPanelController&) = delete;
  SidebarWebPanelController& operator=(const SidebarWebPanelController&) =
      delete;

  // |suppress_animations| is for the case where the web panel is taking the
  // side panel's place: the two share the same space, so they have to swap in
  // a single frame rather than one animating over the other.
  void ToggleWebPanel(const SidebarItem& item, bool suppress_animations);

  // True only while a panel is open and not animating closed. Callers asking
  // "is a panel on offer to the user" want this.
  bool HasOpenPanel() const;

  // True while the panel's tab is still in the strip, including during a close
  // animation. Only teardown needs this - it has to reach a panel that is
  // still animating out.
  bool HasPanelTab() const;

  // Animates the panel closed, then lets the page run its unload handlers, so
  // the tab outlives this call. |suppress_animations| skips the animation, for
  // the case where the side panel is taking this panel's place - see
  // ToggleWebPanel() for the other direction of the same swap.
  void CloseWebPanel(bool suppress_animations = false);

  // Skips unload handlers, so the tab is gone by the time this returns.
  void CloseWebPanelForShutdown();

  const SidebarItem& panel_item() const { return panel_item_; }
  const content::WebContents* panel_contents() const {
    return panel_contents_.get();
  }

 private:
  enum class State { kClosed, kOpen, kClosing };

  BraveMultiContentsView* GetMultiContentsView();
  const BraveMultiContentsView* GetMultiContentsView() const;

  void OpenWebPanel(const SidebarItem& item, bool animate);

  // Closes the panel's tab right now, skipping the animation. Used when the
  // panel is being replaced, where the strip width does not change.
  void CloseWebPanelImmediately();

  // Closes the panel's tab once the close animation has finished.
  void FinishClose();

  void ClearPanelState();

  // TabStripModelObesrver:
  void OnTabWillBeRemoved(tabs::TabHandle tab) override;

  raw_ref<BrowserView> browser_view_;
  base::RepeatingClosure web_panel_state_changed_;
  raw_ptr<content::WebContents> panel_contents_ = nullptr;
  sidebar::SidebarItem panel_item_;
  State state_ = State::kClosed;
  base::WeakPtrFactory<SidebarWebPanelController> weak_factory_{this};
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_WEB_PANEL_CONTROLLER_H_
