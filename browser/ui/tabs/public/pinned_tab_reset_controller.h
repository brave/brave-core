/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_PINNED_TAB_RESET_CONTROLLER_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_PINNED_TAB_RESET_CONTROLLER_H_

#include <map>
#include <optional>
#include <string>

#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"
#include "url/gurl.h"

class BrowserWindowInterface;

namespace content {
class WebContents;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace ui {
class MouseEvent;
}  // namespace ui

// Remembers the URL each pinned tab was pinned with and resets the tab to it,
// instead of closing the tab, when the user presses the close tab shortcut or
// double-clicks the tab. Right-click -> "Close tab" still closes it.
class PinnedTabResetController : public TabStripModelObserver {
 public:
  DECLARE_USER_DATA(PinnedTabResetController);

  explicit PinnedTabResetController(BrowserWindowInterface& browser);
  PinnedTabResetController(const PinnedTabResetController&) = delete;
  PinnedTabResetController& operator=(const PinnedTabResetController&) = delete;
  ~PinnedTabResetController() override;

  // Returns the instance owned by `browser`, or nullptr when the feature is
  // off or `browser` isn't a normal window.
  static PinnedTabResetController* From(BrowserWindowInterface* browser);

  // Handles the close tab shortcut when the selection contains a pinned tab:
  // pinned tabs are reset, the other selected tabs are closed, and the most
  // recently used unpinned tab (or a new tab) becomes active. Returns false,
  // doing nothing, when the shortcut should close tabs as usual.
  bool HandleCloseTabShortcut();

  // Navigates `tab` to the URL it was pinned with, clearing its history.
  void ResetPinnedTab(tabs::TabInterface* tab);

  // Used when a tab is restored from a session or from the recently closed
  // list.
  void RestorePinnedUrl(content::WebContents* contents, const GURL& url);

  // Adds kBravePinnedTabUrlKey to `extra_data` when the tab at `index`, a
  // valid index in this window's tab strip, has a pinned URL, and leaves
  // `extra_data` untouched otherwise. Used for recently closed tab entries,
  // read back by pinned_tab_reset::MaybeRestoreFromExtraData().
  void PopulateExtraData(int index,
                         std::map<std::string, std::string>& extra_data) const;

 private:
  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  void OnTabPinnedStateChanged(tabs::TabInterface* tab, int index) override;

  void RecordPinnedUrl(content::WebContents* contents);

  const raw_ref<BrowserWindowInterface> browser_;

  ui::ScopedUnownedUserData<PinnedTabResetController> scoped_unowned_user_data_;
};

namespace pinned_tab_reset {

// The URL `contents` was pinned with, if it's a pinned tab that has one.
std::optional<GURL> GetPinnedUrl(const content::WebContents* contents);

// Keeps `contents` from ever getting a pinned URL. For contents that only
// stand in for a pinned tab, like shared pinned tab placeholders in inactive
// windows.
void ExcludeFromPinnedTabReset(content::WebContents* contents);

// Hooks for the tab views, and the session and recently closed tab code. All
// are no-ops when the window has no controller.

// Resets `tab` when `event` is a plain left double-click on a pinned tab.
void MaybeResetOnMousePressed(tabs::TabInterface* tab,
                              const ui::MouseEvent& event);
void MaybePopulateExtraData(BrowserWindowInterface& browser,
                            int index,
                            std::map<std::string, std::string>& extra_data);
void MaybeRestoreFromExtraData(
    BrowserWindowInterface* browser,
    content::WebContents* restored_contents,
    const std::map<std::string, std::string>& extra_data);

}  // namespace pinned_tab_reset

#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_PINNED_TAB_RESET_CONTROLLER_H_
