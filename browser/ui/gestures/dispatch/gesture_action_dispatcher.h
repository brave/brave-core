// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_DISPATCH_GESTURE_ACTION_DISPATCHER_H_
#define BRAVE_BROWSER_UI_GESTURES_DISPATCH_GESTURE_ACTION_DISPATCHER_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/callback_list.h"
#include "base/functional/callback.h"
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
class RenderFrameHost;
class WebContents;
}  // namespace content

class GURL;
class Profile;
class BrowserWindowInterface;

namespace gestures {

// Dispatches the chained actions a gesture, rocker, or wheel event resolved to.
class GestureActionDispatcher final {
 public:
  explicit GestureActionDispatcher(tabs::TabInterface& tab);

  GestureActionDispatcher(const GestureActionDispatcher&) = delete;
  GestureActionDispatcher& operator=(const GestureActionDispatcher&) = delete;

  ~GestureActionDispatcher();

  // `callback` runs once every chained action actually finishes,
  // including async ones (e.g. navigation), not once they're all posted.
  void DispatchActionsAsync(
      GestureActions actions,
      std::optional<SmartGestureContentInfo> smart_gesture_content,
      base::OnceClosure callback);

 private:
  // Runs one action, then re-posts itself for the rest, so each gets its own
  // message-loop turn and sees the previous action's effects fully settled.
  void DispatchActions(GestureActions actions);

  // Async cases run `callback` themselves; synchronous ones fall through to
  // `callback.Run()` below.
  void HandleGestureAction(const GestureActionInfo& action,
                           base::OnceClosure callback);
  // Re-posts `DispatchActions` for `remaining_actions` once the action that
  // just ran has fully finished (e.g. a navigation actually committing).
  void HandleGestureActionCallback(GestureActions actions);
  void MaybeRunAllActionsDoneCallback();

  void HandleNavigateToUrl(const GestureActionInfo& action,
                           base::OnceClosure callback);

  // Navigates to a Brave Search or Ask Brave results page for the active
  // tab's selected text, per `action`'s param type, if non-empty.
  void HandleSearchSelectedText(const GestureActionInfo& action,
                                base::OnceClosure callback);
  void HandleSearchSelectedTextCallback(
      SmartGestureSelectedTextSearchCommandParamType command_param_type,
      base::OnceClosure callback,
      const std::string& selected_text);

  // Reads the active tab's currently focused frame's selected text, then, if
  // non-empty, copies it to the clipboard.
  void HandleCopySelectedText(base::OnceClosure callback);
  void HandleCopySelectedTextCallback(base::OnceClosure callback,
                                      const std::string& selected_text);

  // The following all read `smart_gesture_content_`; a no-op if it doesn't
  // match the content type each expects.
  void HandleOpenLinkIn(const GestureActionInfo& action);
  void HandleCopyLinkAddress();
  void HandleOpenImageIn(const GestureActionInfo& action);
  void HandleSaveImage();
  void HandleCopyImage();
  // Shared by `HandleOpenLinkIn`/`HandleOpenImageIn`; attributes the
  // navigation to `content`'s captured frame, not the browser.
  void OpenUrlIn(const GURL& url,
                 const SmartGestureContentInfo& content,
                 const GestureActionInfo& action);

  void HandleSwitchToTab(const GestureActionInfo& action);

  void HandleOpenSettingsPage(const GestureActionInfo& action);

  // Whether `browser_command_id` should run now; `IDC_BACK`/`IDC_FORWARD`
  // need a fresh check since their cached enabled bit can be briefly stale.
  bool ShouldHandleBrowserCommand(int browser_command_id) const;

  // Every command not specially handled above, run as a plain `IDC_*`
  // browser command.
  void HandleBrowserCommand(const GestureActionInfo& action);

  // Binds `gesture_text_selection_extractor_` to the focused frame for its
  // selected text, or runs `no_focused_frame_callback` if there's none.
  void GetSelectedText(
      base::OnceCallback<void(const std::string&)> selected_text_callback,
      base::OnceClosure no_focused_frame_callback);
  // Returns `smart_gesture_content_` if set and of `type`, else null; shared
  // by every `Handle*` above that only acts on one content type.
  const SmartGestureContentInfo* GetSmartGestureContent(
      SmartGestureContentType type) const;

  // Read fresh each time, never cached, since an earlier chained action can
  // change which tab is active before a later one runs.
  content::WebContents* GetActiveWebContents();

  // Returns `GetActiveWebContents()`'s focused frame, or null if there's no
  // active tab or focused frame.
  content::RenderFrameHost* GetFocusedRenderFrameHost();

  // Returns null once `tab_` has been cleared by `TabWillDetachCallback`.
  BrowserWindowInterface* GetBrowserWindowInterface() const;
  Profile* GetProfile() const;

  // Activates the first open tab matching `pattern`; repeating the same
  // pattern cycles to the next match instead of reactivating the current one.
  void SwitchToMatchingTab(const std::string& pattern);

  // Returns the index into `matched_tabs` to activate, the one after the
  // previous match if `pattern` repeats and still matches, else the first.
  size_t IndexOfNextMatchedTabToActivate(const MatchedTabs& matched_tabs,
                                         std::string_view pattern) const;

  // Clears `tab_` only for `kDelete`; `kInsertIntoOtherWindow` leaves it
  // valid, since every method that reads `tab_` treats null as a no-op.
  void TabWillDetachCallback(tabs::TabInterface* tab,
                             tabs::TabInterface::DetachReason detach_reason);

  raw_ptr<tabs::TabInterface> tab_;
  base::CallbackListSubscription tab_will_detach_subscription_;

  std::string switch_to_tab_cycle_pattern_;
  base::WeakPtr<content::WebContents> switch_to_tab_cycle_web_contents_;

  // Kept alive here for the renderer's reply to arrive; a local variable
  // would be destroyed, dropping the connection, before it could respond.
  mojo::Remote<mojom::GestureTextSelectionExtractor>
      gesture_text_selection_extractor_;

  // Set by `DispatchActionsAsync` for the in-flight chain; read by whichever
  // `Handle*` method needs the link/image/selection a smart gesture captured.
  std::optional<SmartGestureContentInfo> smart_gesture_content_;

  // Set by `DispatchActionsAsync` for the in-flight chain; run once every
  // chained action has actually finished.
  base::OnceClosure all_actions_done_callback_;

  base::WeakPtrFactory<GestureActionDispatcher> weak_factory_{this};
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_DISPATCH_GESTURE_ACTION_DISPATCHER_H_
