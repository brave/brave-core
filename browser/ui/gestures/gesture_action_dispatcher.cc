// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_action_dispatcher.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/gestures/gesture_command_browser_mapping.h"
#include "brave/browser/ui/gestures/gesture_tab_match_info.h"
#include "brave/browser/ui/gestures/gesture_tab_match_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_types.h"
#include "brave/components/gestures/core/common/actions/navigate_to_url_params_info.h"
#include "brave/components/gestures/core/common/actions/switch_to_tab_params_info.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "content/public/common/url_constants.h"
#include "ui/base/base_window.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace gestures {

GestureActionDispatcher::GestureActionDispatcher(tabs::TabInterface& tab)
    : tab_(tab) {}

GestureActionDispatcher::~GestureActionDispatcher() = default;

void GestureActionDispatcher::DispatchActionsAsync(
    std::vector<GestureActionInfo> actions) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostNonNestableTask(
      FROM_HERE,
      base::BindOnce(&GestureActionDispatcher::DispatchActions,
                     weak_factory_.GetWeakPtr(), std::move(actions)));
}

void GestureActionDispatcher::DispatchActions(
    std::vector<GestureActionInfo> actions) {
  if (actions.empty()) {
    return;
  }
  RunGestureAction(actions.front());
  actions.erase(actions.begin());
  if (!actions.empty()) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostNonNestableTask(
        FROM_HERE,
        base::BindOnce(&GestureActionDispatcher::DispatchActions,
                       weak_factory_.GetWeakPtr(), std::move(actions)));
  }
}

void GestureActionDispatcher::RunGestureAction(
    const GestureActionInfo& action) {
  switch (action.command) {
    case GestureCommandType::kNavigateToUrl:
      RunNavigateToUrl(action);
      return;
    case GestureCommandType::kScrollToTop:
      RunScrollToTop();
      return;
    case GestureCommandType::kSwitchToTab:
      RunSwitchToTab(action);
      return;
    default:
      break;
  }
  RunBrowserCommand(action);
}

void GestureActionDispatcher::RunNavigateToUrl(
    const GestureActionInfo& action) {
  const auto* navigate_to_url_action =
      std::get_if<NavigateToUrlParamsInfo>(&action.command_params);
  if (!navigate_to_url_action) {
    return;
  }
  const GURL url(navigate_to_url_action->url_spec);
  // Treated as untrusted, synced, user-entered text, so restricted to
  // http/https for ordinary web pages and `brave:` for any of Brave's own
  // internal pages (settings, wallet, etc. are all browser-trusted surfaces,
  // not just a specific allowlisted subset). This can't be used to run a
  // `javascript:` URL or load a local file.
  if (content::WebContents* active_web_contents = GetActiveWebContents();
      active_web_contents && url.is_valid() &&
      (url.SchemeIsHTTPOrHTTPS() || url.SchemeIs(content::kBraveUIScheme))) {
    active_web_contents->GetController().LoadURL(
        url, content::Referrer(), ui::PAGE_TRANSITION_TYPED, std::string());
  }
}

void GestureActionDispatcher::RunScrollToTop() {
  if (content::WebContents* active_web_contents = GetActiveWebContents()) {
    active_web_contents->ScrollToTopOfDocument();
  }
}

void GestureActionDispatcher::RunSwitchToTab(const GestureActionInfo& action) {
  const auto* switch_to_tab_action =
      std::get_if<SwitchToTabParamsInfo>(&action.command_params);
  if (switch_to_tab_action) {
    SwitchToMatchingTab(switch_to_tab_action->pattern);
  }
}

void GestureActionDispatcher::RunBrowserCommand(
    const GestureActionInfo& action) {
  const std::optional<int> command_id = MaybeToGestureCommandId(action.command);
  if (!command_id) {
    return;
  }
  // A tab can move between browser windows, so, as in `sad_tab.cc`, look up
  // the current one at dispatch time rather than caching it. Null only
  // during the brief window where a tab is detached while being dragged to
  // another window; see `TabInterface::GetBrowserWindowInterface()`.
  BrowserWindowInterface* const browser_window =
      tab_->GetBrowserWindowInterface();
  if (!browser_window) {
    return;
  }
  // `ExecuteCommand`'s own enabled check for `IDC_BACK`/`IDC_FORWARD` relies
  // on the toolbar's `CommandUpdater`, which only refreshes on a navigation
  // state notification; checking the `NavigationController` directly here
  // instead avoids acting on a momentarily stale cached enabled bit.
  if (action.command == GestureCommandType::kBack &&
      !chrome::CanGoBack(browser_window)) {
    return;
  }
  if (action.command == GestureCommandType::kForward &&
      !chrome::CanGoForward(browser_window)) {
    return;
  }
  // `ExecuteCommand` returns false if the command is currently disabled (e.g.
  // `IDC_BACK`/`IDC_FORWARD` with no history in that direction), the same as
  // if the real toolbar button were greyed out, not a gesture-detection
  // failure.
  chrome::ExecuteCommand(browser_window, *command_id);
}

content::WebContents* GestureActionDispatcher::GetActiveWebContents() {
  BrowserWindowInterface* const browser_window =
      tab_->GetBrowserWindowInterface();
  if (!browser_window) {
    return nullptr;
  }
  tabs::TabInterface* const active_tab =
      browser_window->GetActiveTabInterface();
  return active_tab ? active_tab->GetContents() : nullptr;
}

void GestureActionDispatcher::SwitchToMatchingTab(const std::string& pattern) {
  // An empty pattern should never match, e.g. an accidentally-cleared
  // switch-to-tab pattern shouldn't match every tab with an empty title.
  if (pattern.empty()) {
    return;
  }

  const std::vector<TabMatchInfo> tab_matches =
      FindMatchingTabs(*tab_->GetProfile(), pattern);
  if (tab_matches.empty()) {
    return;
  }

  const auto& [browser_window, index] =
      tab_matches[IndexOfNextMatchToActivate(tab_matches, pattern)];
  TabStripModel* const tab_strip_model = browser_window->GetTabStripModel();
  tab_strip_model->ActivateTabAt(index);
  if (ui::BaseWindow* const window = browser_window->GetWindow()) {
    window->Activate();
  }

  last_switch_to_tab_pattern_ = pattern;
  last_switch_to_tab_web_contents_ =
      tab_strip_model->GetWebContentsAt(index)->GetWeakPtr();
}

size_t GestureActionDispatcher::IndexOfNextMatchToActivate(
    const std::vector<TabMatchInfo>& tab_matches,
    const std::string& pattern) const {
  if (pattern != last_switch_to_tab_pattern_ ||
      !last_switch_to_tab_web_contents_) {
    return 0;
  }

  const auto it = std::ranges::find_if(
      tab_matches.cbegin(), tab_matches.cend(), [&](const TabMatchInfo& match) {
        return match.browser_window->GetTabStripModel()->GetWebContentsAt(
                   match.tab_index) == last_switch_to_tab_web_contents_.get();
      });
  if (it == tab_matches.cend()) {
    return 0;
  }
  return (static_cast<size_t>(it - tab_matches.cbegin()) + 1) %
         tab_matches.size();
}

}  // namespace gestures
