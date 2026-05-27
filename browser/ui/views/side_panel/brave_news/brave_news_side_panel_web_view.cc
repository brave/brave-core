// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/brave_news/brave_news_side_panel_web_view.h"

#include "base/check.h"
#include "brave/browser/ui/webui/brave_news/brave_news_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace {

BrowserWindowInterface* FindNormalBrowser(
    const content::BrowserContext* context) {
  BrowserWindowInterface* normal_browser = nullptr;
  ForEachCurrentBrowserWindowInterfaceOrderedByActivation(
      [&](BrowserWindowInterface* browser) {
        if (browser->GetType() == BrowserWindowInterface::TYPE_NORMAL &&
            browser->GetProfile() == context) {
          normal_browser = browser;
          return false;
        }
        return true;
      });
  return normal_browser;
}

}  // namespace

using SidePanelWebUIViewT_BraveNewsUI = SidePanelWebUIViewT<BraveNewsUI>;
BEGIN_TEMPLATE_METADATA(SidePanelWebUIViewT_BraveNewsUI, SidePanelWebUIViewT)
END_METADATA

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
  auto* browser_view = BrowserView::GetBrowserViewForNativeWindow(
      GetWidget()->GetNativeWindow());
  if (!browser_view) {
    return nullptr;
  }
  auto* browser = browser_view->browser();

  // Always navigate the main browser's active tab, regardless of disposition.
  NavigateParams params(browser, target_url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::CURRENT_TAB;
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
  auto* browser = FindNormalBrowser(source->GetBrowserContext());
  if (!browser) {
    return WebUIContentsWrapper::Host::OpenURLFromTab(
        source, params, std::move(navigation_handle_callback));
  }

  // Always navigate the main browser's active tab — clicks (including
  // ctrl/middle-click, window.open, etc.) never spawn new tabs/windows from
  // the sidebar.
  content::OpenURLParams new_params(params);
  new_params.disposition = WindowOpenDisposition::CURRENT_TAB;
  return browser->OpenURL(new_params, std::move(navigation_handle_callback));
}
