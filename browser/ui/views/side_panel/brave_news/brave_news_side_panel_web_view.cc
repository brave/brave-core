// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/brave_news/brave_news_side_panel_web_view.h"

#include "base/check.h"
#include "brave/browser/ui/webui/brave_news/brave_news_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

using SidePanelWebUIViewT_BraveNewsUI = SidePanelWebUIViewT<BraveNewsUI>;
BEGIN_TEMPLATE_METADATA(SidePanelWebUIViewT_BraveNewsUI, SidePanelWebUIViewT)
END_METADATA

namespace {

// The sidebar requests a new tab either by opening a `_blank` link (which
// arrives as a new-tab disposition) or via `window.open()`. Everything else
// (plain link clicks) should navigate the main browser's active tab.
bool ShouldOpenInNewTab(WindowOpenDisposition disposition) {
  return disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB ||
         disposition == WindowOpenDisposition::NEW_BACKGROUND_TAB;
}

}  // namespace

// static
std::unique_ptr<views::View> BraveNewsSidePanelWebView::CreateView(
    Profile* profile,
    SidePanelEntryScope& scope) {
  CHECK(profile);
  auto web_view = std::make_unique<BraveNewsSidePanelWebView>(
      scope, std::make_unique<WebUIContentsWrapperT<BraveNewsUI>>(
                 GURL(kBraveNewsURL), profile, IDS_BRAVE_NEWS_TITLE,
                 /*esc_closes_ui=*/false));
  web_view->ShowUI();
  return web_view;
}

BraveNewsSidePanelWebView::BraveNewsSidePanelWebView(
    SidePanelEntryScope& scope,
    std::unique_ptr<WebUIContentsWrapperT<BraveNewsUI>> contents_wrapper)
    : SidePanelWebUIViewT<BraveNewsUI>(scope,
                                       base::RepeatingClosure(),
                                       base::RepeatingClosure(),
                                       std::move(contents_wrapper)) {}

BraveNewsSidePanelWebView::~BraveNewsSidePanelWebView() = default;

content::WebContents* BraveNewsSidePanelWebView::AddNewContents(
    content::WebContents* source,
    std::unique_ptr<content::WebContents> new_contents,
    const GURL& target_url,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& window_features,
    bool user_gesture,
    bool* was_blocked) {
  auto* browser = webui::GetBrowserWindowInterface(source);
  if (!browser) {
    return nullptr;
  }

  // `window.open()` from the sidebar (opening an article in a new tab, or a
  // ctrl/middle-click) opens a new tab in the main browser window rather than
  // spawning a popup attached to the side panel; any other request navigates
  // the main browser's active tab.
  NavigateParams params(browser, target_url, ui::PAGE_TRANSITION_LINK);
  params.disposition = ShouldOpenInNewTab(disposition)
                           ? WindowOpenDisposition::NEW_FOREGROUND_TAB
                           : WindowOpenDisposition::CURRENT_TAB;
  params.window_action = NavigateParams::WindowAction::kNoAction;
  params.user_gesture = user_gesture;
  Navigate(&params);
  return params.navigated_or_inserted_contents;
}

content::WebContents* BraveNewsSidePanelWebView::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params,
    base::OnceCallback<void(content::NavigationHandle&)>
        navigation_handle_callback) {
  auto* browser = webui::GetBrowserWindowInterface(source);
  if (!browser) {
    return WebUIContentsWrapper::Host::OpenURLFromTab(
        source, params, std::move(navigation_handle_callback));
  }

  // Route the navigation to the main browser window rather than the side
  // panel's own WebContents. New-tab requests (opening articles in a new tab,
  // or ctrl/middle-clicking a link) are honored as a new tab in that window;
  // everything else navigates its active tab.
  content::OpenURLParams new_params(params);
  if (!ShouldOpenInNewTab(new_params.disposition)) {
    new_params.disposition = WindowOpenDisposition::CURRENT_TAB;
  }
  return browser->OpenURL(new_params, std::move(navigation_handle_callback));
}
