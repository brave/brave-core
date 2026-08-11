/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_talk/brave_talk_side_panel_web_view.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/ui/views/side_panel/brave_talk/brave_talk_side_panel_navigation_throttle.h"
#include "brave/components/sidebar/browser/constants.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/side_panel/side_panel_content_proxy.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_scope.h"
#include "chrome/browser/ui/side_panel/side_panel_util.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "components/permissions/permission_request_manager.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

BEGIN_METADATA(BraveTalkSidePanelWebView)
END_METADATA

// static
std::unique_ptr<views::View> BraveTalkSidePanelWebView::CreateView(
    Profile* profile,
    SidePanelEntryScope& scope) {
  CHECK(profile);

  auto web_view = std::make_unique<BraveTalkSidePanelWebView>(profile);

  auto web_contents =
      content::WebContents::Create(content::WebContents::CreateParams(profile));

  // The side panel's WebContents is not a tab, so the tab helpers that
  // normally create a PermissionRequestManager don't run. Create one
  // explicitly so mic/camera permission prompts can be shown (mirrors
  // ContextualTasksSidePanelCoordinator).
  permissions::PermissionRequestManager::CreateForWebContents(
      web_contents.get());

  // Lets `BraveTalkSidePanelNavigationThrottle` recognise this contents and
  // keep it pinned to the Brave Talk origin. Must be attached before the
  // initial load below, so that load is seen by the throttle.
  BraveTalkSidePanelContents::CreateForWebContents(web_contents.get());

  // Wire up the embedding context so the hosting browser resolves from the
  // panel's contents. `GetBrowserWindowInterface()` is valid for either scope
  // type, and this entry is only ever registered globally.
  webui::SetBrowserWindowInterface(web_contents.get(),
                                   &scope.GetBrowserWindowInterface());

  web_contents->GetController().LoadURLWithParams(
      content::NavigationController::LoadURLParams(
          GURL(sidebar::kBraveTalkURL)));

  // Use SetOwnedWebContents (not SetWebContents) so the WebView sets itself as
  // the WebContentsDelegate — required for our RequestMediaAccessPermission /
  // OpenURLFromTab / AddNewContents overrides to be called.
  web_view->SetOwnedWebContents(std::move(web_contents));

  // Mark the WebContents as visible so that PermissionRequestManager considers
  // it an active tab and will show mic/camera permission prompts. Without this,
  // the WebContents stays HIDDEN and `ShowPrompt` bails out early.
  web_view->web_contents()->WasShown();

  // The contents is ready immediately (no WebUI bootstrap to await).
  SidePanelUtil::GetSidePanelContentProxy(web_view.get())->SetAvailable(true);

  return web_view;
}

BraveTalkSidePanelWebView::BraveTalkSidePanelWebView(Profile* profile)
    : views::WebView(profile) {
  // Use the shared side panel web view id so the hosted contents is
  // discoverable by the same lookups the wrapper-based views support (e.g.
  // `SidePanelCoordinator::GetWebContentsForTest`).
  SetID(SidePanelWebUIView::kSidePanelWebViewId);
}

BraveTalkSidePanelWebView::~BraveTalkSidePanelWebView() = default;

content::WebContents* BraveTalkSidePanelWebView::AddNewContents(
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

  // `window.open()` from the panel (or a ctrl/middle-click) opens a new tab in
  // the main browser window rather than spawning a popup attached to the side
  // panel.
  NavigateParams params(browser, target_url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.window_action = NavigateParams::WindowAction::kNoAction;
  params.user_gesture = user_gesture;
  Navigate(&params);
  return params.navigated_or_inserted_contents;
}

content::WebContents* BraveTalkSidePanelWebView::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params,
    base::OnceCallback<void(content::NavigationHandle&)>
        navigation_handle_callback) {
  // Anything asking for a new tab or window is handed to the main browser.
  // Same-tab requests fall through to the panel's own WebContents, where
  // `BraveTalkSidePanelNavigationThrottle` decides whether the destination may
  // render in the panel. Note that same-tab navigations mostly do not reach
  // this method at all — Blink starts those directly rather than deferring to
  // the delegate — which is why the origin check lives in the throttle.
  if (params.disposition != WindowOpenDisposition::CURRENT_TAB) {
    if (auto* browser = webui::GetBrowserWindowInterface(source)) {
      return browser->OpenURL(params, std::move(navigation_handle_callback));
    }
  }
  return views::WebView::OpenURLFromTab(source, params,
                                        std::move(navigation_handle_callback));
}

void BraveTalkSidePanelWebView::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback) {
  MediaCaptureDevicesDispatcher::GetInstance()->ProcessMediaAccessRequest(
      web_contents, request, std::move(callback),
      /*extension=*/nullptr);
}

bool BraveTalkSidePanelWebView::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const url::Origin& security_origin,
    blink::mojom::MediaStreamType type) {
  return MediaCaptureDevicesDispatcher::GetInstance()
      ->CheckMediaAccessPermission(render_frame_host, security_origin, type,
                                   /*extension=*/nullptr);
}
