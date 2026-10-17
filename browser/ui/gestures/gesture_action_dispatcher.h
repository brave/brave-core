// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_ACTION_DISPATCHER_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_ACTION_DISPATCHER_H_

#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/gestures/gesture_tab_match_info.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"

namespace content {
class WebContents;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace gestures {

// Runs the chained actions a gesture, rocker, or wheel event resolved to.
class GestureActionDispatcher final {
 public:
  explicit GestureActionDispatcher(tabs::TabInterface& tab);

  GestureActionDispatcher(const GestureActionDispatcher&) = delete;
  GestureActionDispatcher& operator=(const GestureActionDispatcher&) = delete;

  ~GestureActionDispatcher();

  // Waits until the current input event has finished being handled before
  // running the first action, rather than running it partway through.
  void DispatchActionsAsync(std::vector<GestureActionInfo> actions);

 private:
  // Runs one action, then re-posts itself for the remainder so each action
  // in a chain gets its own full message-loop turn: a command like
  // `IDC_NEW_TAB` doesn't necessarily finish settling its effects (e.g. which
  // tab is now active) synchronously within its own `ExecuteCommand` call,
  // so a later chained action resolving "the active tab" immediately
  // afterward can still see the pre-command state.
  void DispatchActions(std::vector<GestureActionInfo> actions);
  void RunGestureAction(const GestureActionInfo& gesture_action);
  void RunNavigateToUrl(const GestureActionInfo& gesture_action);
  void RunScrollToTop();
  void RunSwitchToTab(const GestureActionInfo& gesture_action);
  void RunOpenSettings(const GestureActionInfo& gesture_action);

  // Whether `browser_command_id` should actually run right now, for the
  // commands whose own `chrome::ExecuteCommand` enabled check can't be
  // relied on (see this function's own comment in the .cc for why).
  bool ShouldRunBrowserCommand(int browser_command_id) const;

  // Every command not specially handled above, run as a plain `IDC_*`
  // browser command.
  void RunBrowserCommand(const GestureActionInfo& gesture_action);

  // The current tab's `WebContents`, read fresh each time rather than
  // cached, since an earlier action in the same chain (e.g. opening a new
  // tab) can change which tab is active for a later one.
  content::WebContents* GetActiveWebContents();

  // Activates the first open tab, across all browser windows, whose URL or
  // title matches `pattern`. Repeating the same pattern while the
  // previously-activated match is still open and still matches cycles to
  // the next match instead of reactivating the same one, so a gesture
  // mapped to e.g. "*mail*" can step through multiple open Gmail tabs.
  void SwitchToMatchingTab(const std::string& pattern);

  // The index into `tab_matches` to activate: the one after the
  // previously-activated match if `pattern` repeats and that match is still
  // open and still in `tab_matches`, otherwise the first match.
  size_t IndexOfNextMatchToActivate(
      const std::vector<TabMatchInfo>& tab_matches,
      const std::string& pattern) const;

  const raw_ref<tabs::TabInterface> tab_;

  // State for `SwitchToMatchingTab`'s cycling behavior.
  std::string last_switch_to_tab_pattern_;
  base::WeakPtr<content::WebContents> last_switch_to_tab_web_contents_;

  base::WeakPtrFactory<GestureActionDispatcher> weak_factory_{this};
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_ACTION_DISPATCHER_H_
