/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_TALK_BRAVE_TALK_SIDE_PANEL_WEB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_TALK_BRAVE_TALK_SIDE_PANEL_WEB_VIEW_H_

#include <memory>

#include "content/public/browser/media_stream_request.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/webview/webview.h"
#include "url/origin.h"

namespace blink::mojom {
class WindowFeatures;
enum class MediaStreamType;
}  // namespace blink::mojom

namespace content {
class NavigationHandle;
class RenderFrameHost;
class WebContents;
struct OpenURLParams;
}  // namespace content

class Profile;
class SidePanelEntryScope;

// Hosts the remote Brave Talk widget (https://talk.brave.com/widget) inside the
// Chromium side panel. Unlike the WebUI-backed panels (Leo, Wallet, Brave
// News), Brave Talk is a remote HTTPS page, so this view owns a plain
// `WebContents` rather than a `WebUIContentsWrapper`. Link navigations are
// routed to the main browser window, mirroring the Brave News panel behavior.
class BraveTalkSidePanelWebView : public views::WebView {
  METADATA_HEADER(BraveTalkSidePanelWebView, views::WebView)

 public:
  static std::unique_ptr<views::View> CreateView(Profile* profile,
                                                 SidePanelEntryScope& scope);

  BraveTalkSidePanelWebView(const BraveTalkSidePanelWebView&) = delete;
  BraveTalkSidePanelWebView& operator=(const BraveTalkSidePanelWebView&) =
      delete;
  ~BraveTalkSidePanelWebView() override;

  explicit BraveTalkSidePanelWebView(Profile* profile);

 private:
  // content::WebContentsDelegate:
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params,
      base::OnceCallback<void(content::NavigationHandle&)>
          navigation_handle_callback) override;
  content::WebContents* AddNewContents(
      content::WebContents* source,
      std::unique_ptr<content::WebContents> new_contents,
      const GURL& target_url,
      WindowOpenDisposition disposition,
      const blink::mojom::WindowFeatures& window_features,
      bool user_gesture,
      bool* was_blocked) override;

  // content::WebContentsDelegate:
  // Brave Talk needs mic/camera access. Delegate to the browser's media
  // capture devices dispatcher so the user is prompted for permission.
  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      content::MediaResponseCallback callback) override;
  bool CheckMediaAccessPermission(content::RenderFrameHost* render_frame_host,
                                  const url::Origin& security_origin,
                                  blink::mojom::MediaStreamType type) override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_TALK_BRAVE_TALK_SIDE_PANEL_WEB_VIEW_H_
