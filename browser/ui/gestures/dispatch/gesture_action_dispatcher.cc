// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/gesture_action_dispatcher.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/gestures/dispatch/matched_tab_info.h"
#include "brave/browser/ui/gestures/dispatch/navigation_completion_waiter.h"
#include "brave/browser/ui/gestures/dispatch/tab_matcher_util.h"
#include "brave/components/gestures/core/browser/url_navigation_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/int_param_info.h"
#include "brave/components/gestures/core/common/actions/open_settings_page_types.h"
#include "brave/components/gestures/core/common/actions/open_settings_page_web_ui_path_util.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_open_in_command_param_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_util.h"
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
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
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
          base::BindRepeating(&GestureActionDispatcher::TabWillDetachCallback,
                              base::Unretained(this)))) {}

GestureActionDispatcher::~GestureActionDispatcher() = default;

void GestureActionDispatcher::DispatchActionsAsync(
    GestureActions actions,
    std::optional<SmartGestureContentInfo> smart_gesture_content,
    base::OnceClosure all_actions_done_callback) {
  in_flight_dispatch_chains_.push_back(std::make_unique<DispatchChainInfo>());
  DispatchChainInfo* const chain = in_flight_dispatch_chains_.back().get();
  chain->smart_gesture_content = std::move(smart_gesture_content);
  chain->all_actions_done_callback = std::move(all_actions_done_callback);
  if (actions.empty()) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostNonNestableTask(
        FROM_HERE,
        base::BindOnce(&GestureActionDispatcher::MaybeRunAllActionsDoneCallback,
                       weak_factory_.GetWeakPtr(), chain));
    return;
  }
  base::SequencedTaskRunner::GetCurrentDefault()->PostNonNestableTask(
      FROM_HERE,
      base::BindOnce(&GestureActionDispatcher::DispatchActions,
                     weak_factory_.GetWeakPtr(), chain, std::move(actions)));
}

void GestureActionDispatcher::DispatchActions(DispatchChainInfo* dispatch_chain,
                                              GestureActions actions) {
  if (actions.empty()) {
    return;
  }
  GestureActionInfo action = std::move(actions.front());
  actions.erase(actions.cbegin());
  HandleGestureAction(
      dispatch_chain, action,
      base::BindOnce(&GestureActionDispatcher::HandleGestureActionCallback,
                     weak_factory_.GetWeakPtr(), dispatch_chain,
                     std::move(actions)));
}

void GestureActionDispatcher::HandleGestureAction(
    DispatchChainInfo* dispatch_chain,
    const GestureActionInfo& action,
    base::OnceClosure callback) {
  switch (action.command) {
    case kNavigateToUrl:
      HandleNavigateToUrl(action, std::move(callback));
      return;
    case kSearchSelectedText:
      HandleSearchSelectedText(dispatch_chain, action, std::move(callback));
      return;
    case kCopySelectedText:
      HandleCopySelectedText(dispatch_chain, std::move(callback));
      return;
    case kOpenLinkIn:
      HandleOpenLinkIn(dispatch_chain, action);
      break;
    case kCopyLinkAddress:
      HandleCopyLinkAddress(dispatch_chain);
      break;
    case kOpenImageIn:
      HandleOpenImageIn(dispatch_chain, action);
      break;
    case kSaveImage:
      HandleSaveImage(dispatch_chain);
      break;
    case kCopyImage:
      HandleCopyImage(dispatch_chain);
      break;
    case kSwitchToTab:
      HandleSwitchToTab(action);
      break;
    case IDC_OPTIONS:
      HandleOpenSettingsPage(action);
      break;
    default:
      HandleBrowserCommand(action);
      break;
  }
  std::move(callback).Run();
}

void GestureActionDispatcher::HandleGestureActionCallback(
    DispatchChainInfo* dispatch_chain,
    GestureActions actions) {
  if (actions.empty()) {
    MaybeRunAllActionsDoneCallback(dispatch_chain);
    return;
  }
  base::SequencedTaskRunner::GetCurrentDefault()->PostNonNestableTask(
      FROM_HERE, base::BindOnce(&GestureActionDispatcher::DispatchActions,
                                weak_factory_.GetWeakPtr(), dispatch_chain,
                                std::move(actions)));
}

void GestureActionDispatcher::MaybeRunAllActionsDoneCallback(
    DispatchChainInfo* dispatch_chain) {
  // `all_actions_done_callback` may already have run via
  // `TabWillDetachCallback`, e.g. if an action synchronously closed the tab.
  if (dispatch_chain->all_actions_done_callback) {
    std::move(dispatch_chain->all_actions_done_callback).Run();
  }
  std::erase_if(in_flight_dispatch_chains_,
                [dispatch_chain](const auto& entry) {
                  return entry.get() == dispatch_chain;
                });
}

void GestureActionDispatcher::HandleNavigateToUrl(
    const GestureActionInfo& action,
    base::OnceClosure callback) {
  const auto* navigate_to_url_action =
      std::get_if<TextParamInfo>(&action.command_params);
  if (!navigate_to_url_action) {
    std::move(callback).Run();
    return;
  }
  // `FixupURL`, not a plain `GURL`, so a bare domain (e.g. "brave.com")
  // resolves the same way it would from the omnibox, not rejected outright.
  const GURL url = url_formatter::FixupURL(navigate_to_url_action->value,
                                           /*desired_tld=*/"");
  content::WebContents* const active_web_contents = GetActiveWebContents();
  if (!active_web_contents || !IsGestureAllowedToNavigateToUrl(url)) {
    std::move(callback).Run();
    return;
  }
  // Constructed before `LoadURL` so it can't miss a same-document
  // navigation that commits synchronously.
  NavigationCompletionWaiter::Watch(active_web_contents, std::move(callback));
  // No initiator set: `url` comes from the gesture's own configuration, not
  // page content, so this is trusted/browser-initiated like an omnibox entry.
  active_web_contents->GetController().LoadURL(url, content::Referrer(),
                                               ui::PAGE_TRANSITION_TYPED,
                                               /*extra_headers=*/"");
}

void GestureActionDispatcher::HandleSearchSelectedText(
    DispatchChainInfo* dispatch_chain,
    const GestureActionInfo& action,
    base::OnceClosure callback) {
  const auto* command_param = std::get_if<IntParamInfo>(&action.command_params);
  const auto command_param_type =
      command_param
          ? static_cast<SmartGestureSelectedTextSearchCommandParamType>(
                command_param->value)
          : SmartGestureSelectedTextSearchCommandParamType::kBraveSearch;
  // Reuses a smart gesture's own captured selection, avoiding a live
  // re-fetch racing a right-click that can collapse it before dispatch.
  if (const SmartGestureContentInfo* const selected_text_content =
          GetSmartGestureContent(dispatch_chain,
                                 SmartGestureContentType::kSelectedText)) {
    HandleSearchSelectedTextCallback(command_param_type, std::move(callback),
                                     selected_text_content->value);
    return;
  }
  auto [no_focused_frame_callback, selected_text_done_callback] =
      base::SplitOnceCallback(std::move(callback));
  GetSelectedText(
      base::BindOnce(&GestureActionDispatcher::HandleSearchSelectedTextCallback,
                     weak_factory_.GetWeakPtr(), command_param_type,
                     std::move(selected_text_done_callback)),
      std::move(no_focused_frame_callback));
}

void GestureActionDispatcher::HandleSearchSelectedTextCallback(
    SmartGestureSelectedTextSearchCommandParamType command_param_type,
    base::OnceClosure callback,
    const std::string& selected_text) {
  gesture_text_selection_extractor_.reset();
  if (selected_text.empty()) {
    std::move(callback).Run();
    return;
  }
  const GURL url(base::StrCat(
      {ToSearchBaseUrl(command_param_type),
       base::EscapeQueryParamValue(selected_text, /*use_plus=*/true)}));
  content::WebContents* const active_web_contents = GetActiveWebContents();
  if (!active_web_contents) {
    std::move(callback).Run();
    return;
  }
  NavigationCompletionWaiter::Watch(active_web_contents, std::move(callback));
  // No initiator set: `url` is the browser's own search URL built from the
  // selection, like an omnibox entry, not a page-supplied navigation target.
  active_web_contents->GetController().LoadURL(url, content::Referrer(),
                                               ui::PAGE_TRANSITION_TYPED,
                                               /*extra_headers=*/"");
}

void GestureActionDispatcher::HandleCopySelectedText(
    DispatchChainInfo* dispatch_chain,
    base::OnceClosure callback) {
  // See `HandleSearchSelectedText`'s matching comment.
  if (const SmartGestureContentInfo* const selected_text_content =
          GetSmartGestureContent(dispatch_chain,
                                 SmartGestureContentType::kSelectedText)) {
    HandleCopySelectedTextCallback(std::move(callback),
                                   selected_text_content->value);
    return;
  }
  auto [no_focused_frame_callback, selected_text_done_callback] =
      base::SplitOnceCallback(std::move(callback));
  GetSelectedText(
      base::BindOnce(&GestureActionDispatcher::HandleCopySelectedTextCallback,
                     weak_factory_.GetWeakPtr(),
                     std::move(selected_text_done_callback)),
      std::move(no_focused_frame_callback));
}

void GestureActionDispatcher::HandleCopySelectedTextCallback(
    base::OnceClosure callback,
    const std::string& selected_text) {
  gesture_text_selection_extractor_.reset();
  if (Profile* const profile = GetProfile();
      profile && !selected_text.empty()) {
    ui::ScopedClipboardWriter scoped_clipboard_writer(
        ui::ClipboardBuffer::kCopyPaste);
    if (profile->IsOffTheRecord()) {
      scoped_clipboard_writer.MarkAsOffTheRecord();
    }
    scoped_clipboard_writer.WriteText(base::UTF8ToUTF16(selected_text));
  }
  std::move(callback).Run();
}

void GestureActionDispatcher::HandleOpenLinkIn(
    DispatchChainInfo* dispatch_chain,
    const GestureActionInfo& action) {
  const SmartGestureContentInfo* const link_content =
      GetSmartGestureContent(dispatch_chain, SmartGestureContentType::kLink);
  if (!link_content) {
    return;
  }
  OpenUrlIn(GURL(link_content->value), *link_content, action);
}

void GestureActionDispatcher::HandleCopyLinkAddress(
    DispatchChainInfo* dispatch_chain) {
  const SmartGestureContentInfo* const link_content =
      GetSmartGestureContent(dispatch_chain, SmartGestureContentType::kLink);
  if (!link_content) {
    return;
  }
  const GURL url(link_content->value);
  if (!url.is_valid()) {
    return;
  }
  Profile* const profile = GetProfile();
  if (!profile) {
    return;
  }
  // `URLSanitizerService` would create a dependency cycle from here, so the
  // link is always copied exactly as captured, unsanitized.
  ui::ScopedClipboardWriter scoped_clipboard_writer(
      ui::ClipboardBuffer::kCopyPaste);
  if (profile->IsOffTheRecord()) {
    scoped_clipboard_writer.MarkAsOffTheRecord();
  }
  scoped_clipboard_writer.WriteText(base::UTF8ToUTF16(url.spec()));
}

void GestureActionDispatcher::HandleOpenImageIn(
    DispatchChainInfo* dispatch_chain,
    const GestureActionInfo& action) {
  const SmartGestureContentInfo* const image_content =
      GetSmartGestureContent(dispatch_chain, SmartGestureContentType::kImage);
  if (!image_content) {
    return;
  }
  OpenUrlIn(GURL(image_content->value), *image_content, action);
}

void GestureActionDispatcher::HandleSaveImage(
    DispatchChainInfo* dispatch_chain) {
  const SmartGestureContentInfo* const image_content =
      GetSmartGestureContent(dispatch_chain, SmartGestureContentType::kImage);
  if (!image_content) {
    return;
  }
  const GURL url(image_content->value);
  content::RenderFrameHost* const render_frame_host =
      ResolveHitRenderFrameHost(*image_content);
  if (!IsPageSuppliedUrlAllowedToOpen(url) || !render_frame_host) {
    return;
  }
  content::WebContents::FromRenderFrameHost(render_frame_host)
      ->SaveFrameWithHeaders(url, content::Referrer(), /*headers=*/"",
                             /*suggested_filename=*/u"", render_frame_host,
                             /*is_subresource=*/true);
}

void GestureActionDispatcher::HandleCopyImage(
    DispatchChainInfo* dispatch_chain) {
  const SmartGestureContentInfo* const image_content =
      GetSmartGestureContent(dispatch_chain, SmartGestureContentType::kImage);
  if (!image_content) {
    return;
  }
  content::RenderFrameHost* const render_frame_host =
      ResolveHitRenderFrameHost(*image_content);
  if (!render_frame_host) {
    return;
  }
  // `point` is captured in root-frame coordinates; an out-of-process iframe
  // would need it transformed into its own local space first.
  render_frame_host->CopyImageAt(
      base::saturated_cast<int>(image_content->point.x()),
      base::saturated_cast<int>(image_content->point.y()));
}

content::RenderFrameHost* GestureActionDispatcher::ResolveHitRenderFrameHost(
    const SmartGestureContentInfo& content) const {
  if (!content.frame_token) {
    return nullptr;
  }
  content::RenderFrameHost* const render_frame_host =
      content::RenderFrameHost::FromFrameToken(
          content::GlobalRenderFrameHostToken(content.process_id,
                                              *content.frame_token));
  if (!render_frame_host ||
      content::WebContents::FromRenderFrameHost(render_frame_host) !=
          (tab_ ? tab_->GetContents() : nullptr)) {
    // The claimed frame doesn't exist, or belongs to some other tab; a
    // renderer reporting this is compromised.
    return nullptr;
  }
  return render_frame_host;
}

void GestureActionDispatcher::OpenUrlIn(const GURL& url,
                                        const SmartGestureContentInfo& content,
                                        const GestureActionInfo& action) {
  BrowserWindowInterface* const browser_window = GetBrowserWindowInterface();
  if (!browser_window || !IsPageSuppliedUrlAllowedToOpen(url)) {
    return;
  }
  const auto* command_param = std::get_if<IntParamInfo>(&action.command_params);
  const auto command_param_type =
      command_param ? static_cast<SmartGestureOpenInCommandParamType>(
                          command_param->value)
                    : SmartGestureOpenInCommandParamType::kNewBackgroundTab;
  NavigateParams navigate_params(browser_window, url, ui::PAGE_TRANSITION_LINK);
  // `content.origin` is renderer-reported and untrusted; use the resolved
  // frame's own origin instead. Unset for content with no captured frame.
  if (content.frame_token) {
    content::RenderFrameHost* const render_frame_host =
        ResolveHitRenderFrameHost(content);
    if (!render_frame_host) {
      // Don't navigate at all rather than risk misattributing the
      // navigation's initiator; see `ResolveHitRenderFrameHost`.
      return;
    }
    navigate_params.initiator_origin =
        render_frame_host->GetLastCommittedOrigin();
    navigate_params.initiator_frame_token = content.frame_token;
    navigate_params.initiator_process_id = content.process_id;
    navigate_params.initiator_navigation_state =
        render_frame_host->GetCurrentInitiatorNavigationState();
  }
  navigate_params.disposition =
      command_param_type == SmartGestureOpenInCommandParamType::kNewWindow
          ? WindowOpenDisposition::NEW_WINDOW
      : command_param_type ==
              SmartGestureOpenInCommandParamType::kNewForegroundTab
          ? WindowOpenDisposition::NEW_FOREGROUND_TAB
          : WindowOpenDisposition::NEW_BACKGROUND_TAB;
  Navigate(&navigate_params);
}

void GestureActionDispatcher::HandleSwitchToTab(
    const GestureActionInfo& action) {
  if (const auto* const switch_to_tab_action =
          std::get_if<TextParamInfo>(&action.command_params)) {
    SwitchToMatchingTab(switch_to_tab_action->value);
  }
}

void GestureActionDispatcher::HandleOpenSettingsPage(
    const GestureActionInfo& action) {
  const auto* open_settings_page_param =
      std::get_if<IntParamInfo>(&action.command_params);
  // An out-of-range cast just falls through `ToSettingsPageSubPage`'s switch to
  // the basic page, so a corrupted pref value cannot crash here.
  const OpenSettingsPageType open_settings_page_type =
      open_settings_page_param
          ? static_cast<OpenSettingsPageType>(open_settings_page_param->value)
          : OpenSettingsPageType::kBasic;
  if (BrowserWindowInterface* const browser_window =
          GetBrowserWindowInterface()) {
    chrome::ShowSettingsSubPage(browser_window,
                                ToSettingsSubPage(open_settings_page_type));
  }
}

bool GestureActionDispatcher::ShouldHandleBrowserCommand(
    int browser_command_id) const {
  // Bypasses the toolbar's `CommandUpdater` cache, which can be briefly
  // stale right after a navigation, by checking navigation state directly.
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

void GestureActionDispatcher::HandleBrowserCommand(
    const GestureActionInfo& action) {
  // The only sentinel that can reach here via `HandleGestureAction`'s default
  // case; every other one has its own case there and returns earlier.
  if (action.command == kUnassigned) {
    return;
  }
  // A false return means the command is currently disabled, the same as a
  // greyed-out toolbar button, not a gesture-detection failure.
  if (!ShouldHandleBrowserCommand(action.command)) {
    return;
  }
  if (BrowserWindowInterface* const browser_window =
          GetBrowserWindowInterface()) {
    chrome::ExecuteCommand(browser_window, action.command);
  }
}

void GestureActionDispatcher::GetSelectedText(
    base::OnceCallback<void(const std::string&)> selected_text_callback,
    base::OnceClosure no_focused_frame_callback) {
  content::RenderFrameHost* const render_frame_host =
      GetFocusedRenderFrameHost();
  if (!render_frame_host) {
    std::move(no_focused_frame_callback).Run();
    return;
  }
  gesture_text_selection_extractor_.reset();
  render_frame_host->GetRemoteInterfaces()->GetInterface(
      gesture_text_selection_extractor_.BindNewPipeAndPassReceiver());
  gesture_text_selection_extractor_->GetSelectedText(
      std::move(selected_text_callback));
}

const SmartGestureContentInfo* GestureActionDispatcher::GetSmartGestureContent(
    DispatchChainInfo* dispatch_chain,
    SmartGestureContentType type) const {
  const std::optional<SmartGestureContentInfo>& smart_gesture_content =
      dispatch_chain->smart_gesture_content;
  if (!smart_gesture_content || smart_gesture_content->type != type) {
    return nullptr;
  }
  return &*smart_gesture_content;
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

content::RenderFrameHost* GestureActionDispatcher::GetFocusedRenderFrameHost() {
  content::WebContents* const active_web_contents = GetActiveWebContents();
  return active_web_contents ? active_web_contents->GetFocusedFrame() : nullptr;
}

BrowserWindowInterface* GestureActionDispatcher::GetBrowserWindowInterface()
    const {
  return tab_ ? tab_->GetBrowserWindowInterface() : nullptr;
}

Profile* GestureActionDispatcher::GetProfile() const {
  return tab_ ? tab_->GetProfile() : nullptr;
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
      matched_tabs[IndexOfNextMatchedTabToActivate(matched_tabs, pattern)];
  TabStripModel* const tab_strip_model = browser_window->GetTabStripModel();
  tab_strip_model->ActivateTabAt(index);
  if (ui::BaseWindow* const window = browser_window->GetWindow()) {
    window->Activate();
  }

  switch_to_tab_cycle_pattern_ = pattern;
  switch_to_tab_cycle_web_contents_ =
      tab_strip_model->GetWebContentsAt(index)->GetWeakPtr();
}

size_t GestureActionDispatcher::IndexOfNextMatchedTabToActivate(
    const MatchedTabs& matched_tabs,
    std::string_view pattern) const {
  if (pattern != switch_to_tab_cycle_pattern_ ||
      !switch_to_tab_cycle_web_contents_) {
    return 0;
  }

  for (size_t i = 0; i < matched_tabs.size(); ++i) {
    const MatchedTabInfo& matched_tab = matched_tabs.at(i);
    if (matched_tab.browser_window->GetTabStripModel()->GetWebContentsAt(
            matched_tab.index) == switch_to_tab_cycle_web_contents_.get()) {
      return (i + 1) % matched_tabs.size();
    }
  }
  return 0;
}

void GestureActionDispatcher::TabWillDetachCallback(
    tabs::TabInterface* /*tab*/,
    tabs::TabInterface::DetachReason detach_reason) {
  // `kInsertIntoOtherWindow` just moves `tab_` to another window; it and any
  // chain in flight stay valid, so only `kDelete` needs handling here.
  if (detach_reason != tabs::TabInterface::DetachReason::kDelete) {
    return;
  }
  tab_ = nullptr;
  FlushDispatchChains();
}

void GestureActionDispatcher::FlushDispatchChains() {
  for (const auto& dispatch_chain : in_flight_dispatch_chains_) {
    if (dispatch_chain->all_actions_done_callback) {
      std::move(dispatch_chain->all_actions_done_callback).Run();
    }
  }
}

}  // namespace gestures
