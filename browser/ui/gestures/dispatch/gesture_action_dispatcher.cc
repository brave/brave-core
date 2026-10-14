// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/gesture_action_dispatcher.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/gestures/dispatch/gesture_command_browser_util.h"
#include "brave/browser/ui/gestures/dispatch/matched_tab_info.h"
#include "brave/browser/ui/gestures/dispatch/navigation_completion_waiter.h"
#include "brave/browser/ui/gestures/dispatch/open_settings_page_web_ui_path_util.h"
#include "brave/browser/ui/gestures/dispatch/tab_matcher_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/int_param_info.h"
#include "brave/components/gestures/core/common/actions/open_settings_page_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_open_in_command_param_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"
#include "brave/components/gestures/core/common/actions/text_param_info.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/tabs/public/tab_interface.h"
#include "components/url_formatter/url_fixer.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "content/public/common/url_constants.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/base/base_window.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace gestures {

GestureActionDispatcher::GestureActionDispatcher(tabs::TabInterface& tab)
    : tab_(&tab),
      tab_will_detach_subscription_(tab.RegisterWillDetach(
          base::BindRepeating(&GestureActionDispatcher::OnTabWillDetach,
                              base::Unretained(this)))) {}

GestureActionDispatcher::~GestureActionDispatcher() = default;

void GestureActionDispatcher::DispatchActionsAsync(
    GestureActions actions,
    std::optional<SmartGestureContentInfo> smart_gesture_content,
    base::OnceClosure on_all_actions_done) {
  smart_gesture_content_ = std::move(smart_gesture_content);
  on_all_actions_done_ = std::move(on_all_actions_done);
  if (actions.empty()) {
    std::move(on_all_actions_done_).Run();
    return;
  }
  base::SequencedTaskRunner::GetCurrentDefault()->PostNonNestableTask(
      FROM_HERE,
      base::BindOnce(&GestureActionDispatcher::DispatchActions,
                     weak_factory_.GetWeakPtr(), std::move(actions)));
}

void GestureActionDispatcher::DispatchActions(GestureActions actions) {
  if (actions.empty()) {
    return;
  }
  GestureActionInfo action = std::move(actions.front());
  actions.erase(actions.begin());
  RunGestureAction(
      action, base::BindOnce(&GestureActionDispatcher::OnActionDone,
                             weak_factory_.GetWeakPtr(), std::move(actions)));
}

void GestureActionDispatcher::OnActionDone(GestureActions remaining_actions) {
  if (remaining_actions.empty()) {
    std::move(on_all_actions_done_).Run();
    return;
  }
  base::SequencedTaskRunner::GetCurrentDefault()->PostNonNestableTask(
      FROM_HERE,
      base::BindOnce(&GestureActionDispatcher::DispatchActions,
                     weak_factory_.GetWeakPtr(), std::move(remaining_actions)));
}

void GestureActionDispatcher::RunGestureAction(
    const GestureActionInfo& gesture_action,
    base::OnceClosure on_done) {
  switch (gesture_action.command) {
    case kNavigateToUrl:
      RunNavigateToUrl(gesture_action, std::move(on_done));
      return;
    case kSearchSelectedText:
      RunSearchSelectedText(gesture_action, std::move(on_done));
      return;
    case kCopySelectedText:
      RunCopySelectedText(std::move(on_done));
      return;
    case kOpenLinkIn:
      RunOpenLinkIn(gesture_action);
      break;
    case kCopyLinkAddress:
      RunCopyLinkAddress();
      break;
    case kOpenImageIn:
      RunOpenImageIn(gesture_action);
      break;
    case kSaveImage:
      RunSaveImage();
      break;
    case kCopyImage:
      RunCopyImage();
      break;
    case kSwitchToTab:
      RunSwitchToTab(gesture_action);
      break;
    case IDC_OPTIONS:
      RunOpenSettingsPage(gesture_action);
      break;
    default:
      RunBrowserCommand(gesture_action);
      break;
  }
  std::move(on_done).Run();
}

void GestureActionDispatcher::RunNavigateToUrl(
    const GestureActionInfo& gesture_action,
    base::OnceClosure on_done) {
  const auto* navigate_to_url_action =
      std::get_if<TextParamInfo>(&gesture_action.command_params);
  if (!navigate_to_url_action) {
    std::move(on_done).Run();
    return;
  }
  // `FixupURL`, not a plain `GURL`, so a bare domain typed without a scheme
  // (e.g. "brave.com") resolves the same way it would from the omnibox,
  // rather than being rejected outright for having no recognized scheme.
  const GURL url = url_formatter::FixupURL(navigate_to_url_action->value,
                                           /*desired_tld=*/"");
  content::WebContents* const active_web_contents = GetActiveWebContents();
  // Synced, user-entered text, so treated as untrusted: restricted to
  // http/https and Brave's own trusted `brave:` pages, never `javascript:`.
  if (!active_web_contents || !url.is_valid() ||
      !(url.SchemeIsHTTPOrHTTPS() || url.SchemeIs(content::kBraveUIScheme))) {
    std::move(on_done).Run();
    return;
  }
  // Constructed before `LoadURL` so it can't miss a same-document
  // navigation that commits synchronously.
  NavigationCompletionWaiter::Watch(active_web_contents, std::move(on_done));
  active_web_contents->GetController().LoadURL(url, content::Referrer(),
                                               ui::PAGE_TRANSITION_TYPED,
                                               /*extra_headers=*/"");
}

void GestureActionDispatcher::RunSearchSelectedText(
    const GestureActionInfo& gesture_action,
    base::OnceClosure on_done) {
  content::WebContents* const active_web_contents = GetActiveWebContents();
  content::RenderFrameHost* const focused_render_frame_host =
      active_web_contents ? active_web_contents->GetFocusedFrame() : nullptr;
  if (!focused_render_frame_host) {
    std::move(on_done).Run();
    return;
  }
  const auto* command_param =
      std::get_if<IntParamInfo>(&gesture_action.command_params);
  const auto command_param_type =
      command_param
          ? static_cast<SmartGestureSelectedTextSearchCommandParamType>(
                command_param->value)
          : SmartGestureSelectedTextSearchCommandParamType::kBraveSearch;
  text_selection_extractor_.reset();
  focused_render_frame_host->GetRemoteInterfaces()->GetInterface(
      text_selection_extractor_.BindNewPipeAndPassReceiver());
  text_selection_extractor_->GetSelectedText(base::BindOnce(
      &GestureActionDispatcher::SelectedTextCallback,
      weak_factory_.GetWeakPtr(), command_param_type, std::move(on_done)));
}

void GestureActionDispatcher::SelectedTextCallback(
    SmartGestureSelectedTextSearchCommandParamType command_param_type,
    base::OnceClosure on_done,
    const std::string& selected_text) {
  text_selection_extractor_.reset();
  if (selected_text.empty()) {
    std::move(on_done).Run();
    return;
  }
  const std::string_view base_url =
      command_param_type ==
              SmartGestureSelectedTextSearchCommandParamType::kBraveAsk
          ? "https://ask.brave.com/?q="
          : "https://search.brave.com/search?q=";
  const GURL url(
      base::StrCat({base_url, base::EscapeQueryParamValue(selected_text,
                                                          /*use_plus=*/true)}));
  content::WebContents* const active_web_contents = GetActiveWebContents();
  if (!active_web_contents) {
    std::move(on_done).Run();
    return;
  }
  NavigationCompletionWaiter::Watch(active_web_contents, std::move(on_done));
  active_web_contents->GetController().LoadURL(url, content::Referrer(),
                                               ui::PAGE_TRANSITION_TYPED,
                                               /*extra_headers=*/"");
}

void GestureActionDispatcher::RunCopySelectedText(base::OnceClosure on_done) {
  content::WebContents* const active_web_contents = GetActiveWebContents();
  content::RenderFrameHost* const focused_render_frame_host =
      active_web_contents ? active_web_contents->GetFocusedFrame() : nullptr;
  if (!focused_render_frame_host) {
    std::move(on_done).Run();
    return;
  }
  text_selection_extractor_.reset();
  focused_render_frame_host->GetRemoteInterfaces()->GetInterface(
      text_selection_extractor_.BindNewPipeAndPassReceiver());
  text_selection_extractor_->GetSelectedText(
      base::BindOnce(&GestureActionDispatcher::CopySelectedTextCallback,
                     weak_factory_.GetWeakPtr(), std::move(on_done)));
}

void GestureActionDispatcher::CopySelectedTextCallback(
    base::OnceClosure on_done,
    const std::string& selected_text) {
  text_selection_extractor_.reset();
  if (Profile* const profile = GetProfile();
      profile && !selected_text.empty()) {
    ui::ScopedClipboardWriter scw(ui::ClipboardBuffer::kCopyPaste);
    if (profile->IsOffTheRecord()) {
      scw.MarkAsOffTheRecord();
    }
    scw.WriteText(base::UTF8ToUTF16(selected_text));
  }
  std::move(on_done).Run();
}

void GestureActionDispatcher::RunOpenLinkIn(
    const GestureActionInfo& gesture_action) {
  if (!smart_gesture_content_ ||
      smart_gesture_content_->type != SmartGestureContentType::kLink) {
    return;
  }
  OpenSmartUrl(GURL(smart_gesture_content_->value), gesture_action);
}

void GestureActionDispatcher::RunCopyLinkAddress() {
  if (!smart_gesture_content_ ||
      smart_gesture_content_->type != SmartGestureContentType::kLink) {
    return;
  }
  const GURL url(smart_gesture_content_->value);
  if (!url.is_valid()) {
    return;
  }
  Profile* const profile = GetProfile();
  if (!profile) {
    return;
  }
  // `URLSanitizerService` (used by e.g. `CleanAndCopySelectedURL`) only
  // exists inside the monolithic chrome/browser-equivalent target, which
  // `//brave/browser/ui/gestures` can't depend on without a cycle (that
  // target already depends on this one). Always copies the link exactly
  // as captured.
  ui::ScopedClipboardWriter scw(ui::ClipboardBuffer::kCopyPaste);
  if (profile->IsOffTheRecord()) {
    scw.MarkAsOffTheRecord();
  }
  scw.WriteText(base::UTF8ToUTF16(url.spec()));
}

void GestureActionDispatcher::RunOpenImageIn(
    const GestureActionInfo& gesture_action) {
  if (!smart_gesture_content_ ||
      smart_gesture_content_->type != SmartGestureContentType::kImage) {
    return;
  }
  OpenSmartUrl(GURL(smart_gesture_content_->value), gesture_action);
}

void GestureActionDispatcher::OpenSmartUrl(
    const GURL& url,
    const GestureActionInfo& gesture_action) {
  BrowserWindowInterface* const browser_window = GetBrowserWindowInterface();
  // `url` comes directly from page content (a link's `href` or an image's
  // `src`), so it's untrusted in the same way `RunNavigateToUrl`'s synced,
  // user-entered text is: restricted to http/https and Brave's own trusted
  // `brave:` pages, never `javascript:` or other dangerous schemes.
  if (!url.is_valid() || !browser_window ||
      !(url.SchemeIsHTTPOrHTTPS() || url.SchemeIs(content::kBraveUIScheme))) {
    return;
  }
  const auto* command_param =
      std::get_if<IntParamInfo>(&gesture_action.command_params);
  const auto command_param_type =
      command_param ? static_cast<SmartGestureOpenInCommandParamType>(
                          command_param->value)
                    : SmartGestureOpenInCommandParamType::kNewBackgroundTab;
  NavigateParams params(browser_window, url, ui::PAGE_TRANSITION_LINK);
  params.disposition =
      command_param_type == SmartGestureOpenInCommandParamType::kNewWindow
          ? WindowOpenDisposition::NEW_WINDOW
      : command_param_type ==
              SmartGestureOpenInCommandParamType::kNewForegroundTab
          ? WindowOpenDisposition::NEW_FOREGROUND_TAB
          : WindowOpenDisposition::NEW_BACKGROUND_TAB;
  Navigate(&params);
}

void GestureActionDispatcher::RunSaveImage() {
  if (!smart_gesture_content_ ||
      smart_gesture_content_->type != SmartGestureContentType::kImage) {
    return;
  }
  const GURL url(smart_gesture_content_->value);
  content::WebContents* const active_web_contents = GetActiveWebContents();
  content::RenderFrameHost* const focused_render_frame_host =
      active_web_contents ? active_web_contents->GetFocusedFrame() : nullptr;
  if (!url.is_valid() || !focused_render_frame_host) {
    return;
  }
  active_web_contents->SaveFrameWithHeaders(
      url, content::Referrer(), /*headers=*/"",
      /*suggested_filename=*/u"", focused_render_frame_host,
      /*is_subresource=*/true);
}

void GestureActionDispatcher::RunCopyImage() {
  if (!smart_gesture_content_ ||
      smart_gesture_content_->type != SmartGestureContentType::kImage) {
    return;
  }
  content::WebContents* const active_web_contents = GetActiveWebContents();
  content::RenderFrameHost* const focused_render_frame_host =
      active_web_contents ? active_web_contents->GetFocusedFrame() : nullptr;
  if (!focused_render_frame_host) {
    return;
  }
  // `smart_gesture_content_->point` is in the page's root coordinate space
  // (see its own comment); this assumes `focused_render_frame_host` is the
  // main frame, same as every other smart-content action. A focused
  // subframe with its own widget (e.g. an out-of-process iframe) would need
  // that point transformed into the subframe's local space first, same as
  // `GestureInputRegistrar` does for mouse events.
  focused_render_frame_host->CopyImageAt(
      base::saturated_cast<int>(smart_gesture_content_->point.x()),
      base::saturated_cast<int>(smart_gesture_content_->point.y()));
}

void GestureActionDispatcher::RunSwitchToTab(
    const GestureActionInfo& gesture_action) {
  const auto* switch_to_tab_action =
      std::get_if<TextParamInfo>(&gesture_action.command_params);
  if (switch_to_tab_action) {
    SwitchToMatchingTab(switch_to_tab_action->value);
  }
}

void GestureActionDispatcher::RunOpenSettingsPage(
    const GestureActionInfo& gesture_action) {
  const auto* settings_page_param =
      std::get_if<IntParamInfo>(&gesture_action.command_params);
  // An out-of-range cast just falls through `PageToWebUIPath`'s switch to
  // the basic page, so a corrupted pref value cannot crash here.
  const OpenSettingsPageType settings_page =
      settings_page_param
          ? static_cast<OpenSettingsPageType>(settings_page_param->value)
          : OpenSettingsPageType::kBasic;
  if (BrowserWindowInterface* const browser_window =
          GetBrowserWindowInterface()) {
    chrome::ShowSettingsSubPage(browser_window, PageToWebUIPath(settings_page));
  }
}

void GestureActionDispatcher::RunBrowserCommand(
    const GestureActionInfo& gesture_action) {
  if (!ShouldRunBrowserCommand(gesture_action.command)) {
    return;
  }
  std::optional<int> command_id =
      MaybeToGestureCommandId(gesture_action.command);
  if (!command_id) {
    return;
  }
  // A false return means the command is currently disabled, the same as a
  // greyed-out toolbar button, not a gesture-detection failure.
  if (BrowserWindowInterface* const browser_window =
          GetBrowserWindowInterface()) {
    chrome::ExecuteCommand(browser_window, *command_id);
  }
}

bool GestureActionDispatcher::ShouldRunBrowserCommand(
    int browser_command_id) const {
  // Bypasses the toolbar's `CommandUpdater` cache, which can be briefly
  // stale right after a navigation, by checking real navigation state.
  if (browser_command_id != IDC_BACK && browser_command_id != IDC_FORWARD) {
    return true;
  }
  BrowserWindowInterface* const browser_window = GetBrowserWindowInterface();
  if (!browser_window) {
    return false;
  }
  return browser_command_id == IDC_BACK ? chrome::CanGoBack(browser_window)
                                        : chrome::CanGoForward(browser_window);
}

content::WebContents* GestureActionDispatcher::GetActiveWebContents() {
  BrowserWindowInterface* const browser_window = GetBrowserWindowInterface();
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

  Profile* const profile = GetProfile();
  if (!profile) {
    return;
  }
  const MatchedTabs matched_tabs = FindMatchingTabs(*profile, pattern);
  if (matched_tabs.empty()) {
    return;
  }

  const auto& [browser_window, index] =
      matched_tabs[IndexOfNextMatchToActivate(matched_tabs, pattern)];
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
    const MatchedTabs& matched_tabs,
    const std::string& pattern) const {
  if (pattern != last_switch_to_tab_pattern_ ||
      !last_switch_to_tab_web_contents_) {
    return 0;
  }

  const auto iter = std::ranges::find_if(
      matched_tabs.cbegin(), matched_tabs.cend(),
      [&](const MatchedTabInfo& match) {
        return match.browser_window->GetTabStripModel()->GetWebContentsAt(
                   match.index) == last_switch_to_tab_web_contents_.get();
      });
  if (iter == matched_tabs.cend()) {
    return 0;
  }
  return (base::checked_cast<size_t>(iter - matched_tabs.cbegin()) + 1) %
         matched_tabs.size();
}

BrowserWindowInterface* GestureActionDispatcher::GetBrowserWindowInterface()
    const {
  return tab_ ? tab_->GetBrowserWindowInterface() : nullptr;
}

Profile* GestureActionDispatcher::GetProfile() const {
  return tab_ ? tab_->GetProfile() : nullptr;
}

void GestureActionDispatcher::OnTabWillDetach(
    tabs::TabInterface* /*tab*/,
    tabs::TabInterface::DetachReason /*detach_reason*/) {
  tab_ = nullptr;
}

}  // namespace gestures
