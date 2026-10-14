// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_DISPATCH_GESTURE_ACTION_DISPATCHER_H_
#define BRAVE_BROWSER_UI_GESTURES_DISPATCH_GESTURE_ACTION_DISPATCHER_H_

#include <optional>
#include <string>

#include "base/callback_list.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/gestures/dispatch/matched_tab_info.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"
#include "brave/components/gestures/core/common/mojom/gesture_text_selection_extractor.mojom.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_info.h"
#include "components/tabs/public/tab_interface.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
class WebContents;
}  // namespace content

class GURL;
class Profile;
class BrowserWindowInterface;

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
  // `smart_gesture_content` is the link/image/selection captured at a smart
  // gesture's start point, if `actions` resolved from one; `nullopt`
  // otherwise. `on_all_actions_done` runs once every chained action has
  // actually finished, including ones that complete asynchronously (e.g.
  // navigation), not just once they've all been posted.
  void DispatchActionsAsync(
      GestureActions actions,
      std::optional<SmartGestureContentInfo> smart_gesture_content,
      base::OnceClosure on_all_actions_done = base::DoNothing());

 private:
  // Runs one action, then re-posts itself for the rest, so each gets its own
  // message-loop turn and sees the previous action's effects fully settled.
  void DispatchActions(GestureActions actions);

  // Re-posts `DispatchActions` for `remaining_actions`, once whichever action
  // just ran has fully finished; for most actions that's immediate, but e.g.
  // navigating waits for the navigation to actually commit first, so a
  // trailing action like closing the tab can't run against the old page.
  void OnActionDone(GestureActions remaining_actions);

  void RunGestureAction(const GestureActionInfo& gesture_action,
                        base::OnceClosure on_done);
  void RunNavigateToUrl(const GestureActionInfo& gesture_action,
                        base::OnceClosure on_done);
  // Reads the active tab's currently focused frame's selected text, then, if
  // non-empty, navigates to a Brave Search or Brave Ask results page for it,
  // depending on `gesture_action`'s
  // `SmartGestureSelectedTextSearchCommandParamType`.
  void RunSearchSelectedText(const GestureActionInfo& gesture_action,
                             base::OnceClosure on_done);
  void SelectedTextCallback(
      SmartGestureSelectedTextSearchCommandParamType command_param_type,
      base::OnceClosure on_done,
      const std::string& selected_text);
  // Reads the active tab's currently focused frame's selected text, then, if
  // non-empty, copies it to the clipboard.
  void RunCopySelectedText(base::OnceClosure on_done);
  void CopySelectedTextCallback(base::OnceClosure on_done,
                                const std::string& selected_text);
  // The following all read `smart_gesture_content_`, set by the smart gesture
  // that resolved to them; a no-op if it's unset or doesn't match the content
  // type each expects.
  void RunOpenLinkIn(const GestureActionInfo& gesture_action);
  void RunCopyLinkAddress();
  void RunOpenImageIn(const GestureActionInfo& gesture_action);
  // Shared by `RunOpenLinkIn`/`RunOpenImageIn`: opens `url` at the
  // destination `gesture_action`'s `SmartGestureOpenInCommandParamType` names.
  void OpenSmartUrl(const GURL& url, const GestureActionInfo& gesture_action);
  void RunSaveImage();
  void RunCopyImage();
  void RunSwitchToTab(const GestureActionInfo& gesture_action);
  void RunOpenSettingsPage(const GestureActionInfo& gesture_action);

  // Whether `browser_command_id` should run now; `IDC_BACK`/`IDC_FORWARD`
  // need a fresh check since their cached enabled bit can be briefly stale.
  bool ShouldRunBrowserCommand(int browser_command_id) const;

  // Every command not specially handled above, run as a plain `IDC_*`
  // browser command.
  void RunBrowserCommand(const GestureActionInfo& gesture_action);

  // Read fresh each time, never cached, since an earlier chained action can
  // change which tab is active before a later one runs.
  content::WebContents* GetActiveWebContents();

  // Activates the first open tab matching `pattern` by URL or title;
  // repeating the same pattern cycles to the next match instead of
  // reactivating the current one.
  void SwitchToMatchingTab(const std::string& pattern);

  // The index into `matched_tabs` to activate: the one after the previous
  // match if `pattern` repeats and that match still matches, else the first.
  size_t IndexOfNextMatchToActivate(const MatchedTabs& matched_tabs,
                                    const std::string& pattern) const;

  // `nullptr` once `tab_` has been cleared by `OnTabWillDetach`.
  BrowserWindowInterface* GetBrowserWindowInterface() const;
  Profile* GetProfile() const;

  // Clears `tab_`, since a chained action can still be in flight (e.g. a
  // still-navigating action) after the tab it was dispatched for is closed;
  // every method that reads `tab_` treats null as "nothing to act on".
  void OnTabWillDetach(tabs::TabInterface* tab,
                       tabs::TabInterface::DetachReason detach_reason);

  raw_ptr<tabs::TabInterface> tab_;
  base::CallbackListSubscription tab_will_detach_subscription_;

  // State for `SwitchToMatchingTab`'s cycling behavior.
  std::string last_switch_to_tab_pattern_;
  base::WeakPtr<content::WebContents> last_switch_to_tab_web_contents_;

  // Kept alive here for `RunSearchSelectedText`'s reply to arrive; a local
  // variable would be destroyed, dropping the connection, before the
  // renderer could respond.
  mojo::Remote<mojom::GestureTextSelectionExtractor> text_selection_extractor_;

  // Set by `DispatchActionsAsync` for the currently in-flight chain of
  // actions; read by whichever `Run*` method needs the link/image/
  // selection a smart gesture captured.
  std::optional<SmartGestureContentInfo> smart_gesture_content_;

  // Set by `DispatchActionsAsync` for the currently in-flight chain; run by
  // `OnActionDone` once every chained action has actually finished.
  base::OnceClosure on_all_actions_done_;

  base::WeakPtrFactory<GestureActionDispatcher> weak_factory_{this};
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_DISPATCH_GESTURE_ACTION_DISPATCHER_H_
