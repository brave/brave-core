/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_web_contents_delegate.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/brave_file_select_utils.h"
#include "brave/browser/ui/split_view/split_view_link_redirect_utils.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/unload_controller.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"
#include "url/origin.h"

BraveBrowserWebContentsDelegate::BraveBrowserWebContentsDelegate(
    BrowserWindowInterface* browser,
    ExclusiveAccessManager& exclusive_access_manager,
    chrome::BrowserCommandController& command_controller,
    UnloadController& unload_controller,
    web_app::AppBrowserController* app_browser_controller,
    BrowserWindow& window,
    DesktopBrowserWindowCapabilities& capabilities,
    BrowserUiController& browser_ui_controller)
    : BrowserWebContentsDelegate(browser,
                                 exclusive_access_manager,
                                 command_controller,
                                 unload_controller,
                                 app_browser_controller,
                                 window,
                                 capabilities,
                                 browser_ui_controller),
      browser_(*browser),
      unload_controller_(unload_controller) {}

BraveBrowserWebContentsDelegate::~BraveBrowserWebContentsDelegate() = default;

content::WebContents* BraveBrowserWebContentsDelegate::AddNewContents(
    content::WebContents* source,
    std::unique_ptr<content::WebContents> new_contents,
    const GURL& target_url,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& window_features,
    bool user_gesture,
    bool* was_blocked) {
  // For NEW_FOREGROUND_TAB disposition (target="_blank" links) from a source
  // in the left pane of a linked split view, set the split tab ID on the new
  // contents so that SplitViewLinkNavigationThrottle can redirect it to the
  // right pane. If the split tab ID was set, change the disposition to
  // NEW_BACKGROUND_TAB to prevent the empty tab from becoming visible.
  // It'll be closed immediately right after navigation is routed to right
  // pane. By routing at SplitViewLinkNavigationThrottle, proper referrer/opener
  // could be set. These are set after navigation starts. So, can't get it here.
  if (disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB && source &&
      user_gesture && new_contents.get() && !target_url.is_empty()) {
    if (split_view::SetSplitTabIdForRedirect(source, new_contents.get())) {
      disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
    }
  }

  return BrowserWebContentsDelegate::AddNewContents(
      source, std::move(new_contents), target_url, disposition, window_features,
      user_gesture, was_blocked);
}

void BraveBrowserWebContentsDelegate::UpdateTargetURL(
    content::WebContents* source,
    const GURL& url) {
  GURL target_url = url;
  if (url.SchemeIs(content::kChromeUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kBraveUIScheme);
    target_url = target_url.ReplaceComponents(replacements);
  }
  BrowserWebContentsDelegate::UpdateTargetURL(source, target_url);
}

void BraveBrowserWebContentsDelegate::BeforeUnloadFired(
    content::WebContents* source,
    bool proceed,
    bool* proceed_to_fire_unload) {
  // Clear user's choice when user cancelled window closing by beforeunload
  // handler.
  if (!proceed) {
    unload_controller_->set_confirmed_to_close(false);
  }
  BrowserWebContentsDelegate::BeforeUnloadFired(source, proceed,
                                                proceed_to_fire_unload);
}

bool BraveBrowserWebContentsDelegate::ShouldSuppressDialogs(
    content::WebContents* source) {
  auto* tab = tabs::TabInterface::MaybeGetFromContents(source);
  auto* brave_browser =
      static_cast<BraveBrowser*>(browser_->GetBrowserForMigrationOnly());
  return (tab && brave_browser->ShouldIgnoreBeforeUnloadHandlerForTab(
                     tab->GetHandle())) ||
         BrowserWebContentsDelegate::ShouldSuppressDialogs(source);
}

void BraveBrowserWebContentsDelegate::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const blink::mojom::FileChooserParams& params) {
#if BUILDFLAG(IS_ANDROID)
  BrowserWebContentsDelegate::RunFileChooser(render_frame_host, listener,
                                             params);
#else
  auto new_params = params.Clone();
  if (new_params->title.empty()) {
    // Fill title of file chooser with origin of the frame.

    // Note that save mode param is for PPAPI. 'Save As...' or downloading
    // something doesn't reach here. They show 'select file dialog' from
    // DownloadFilePicker::DownloadFilePicker directly.
    // https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/public/mojom/choosers/file_chooser.mojom;l=27;drc=047c7dc4ee1ce908d7fea38ca063fa2f80f92c77
    CHECK(render_frame_host);
    const url::Origin& origin = render_frame_host->GetLastCommittedOrigin();
    new_params->title = brave::GetFileSelectTitle(
        content::WebContents::FromRenderFrameHost(render_frame_host), origin,
        origin,
        params.mode == blink::mojom::FileChooserParams::Mode::kSave
            ? brave::FileSelectTitleType::kSave
            : brave::FileSelectTitleType::kOpen);
  }
  BrowserWebContentsDelegate::RunFileChooser(render_frame_host, listener,
                                             *new_params);
#endif
}
