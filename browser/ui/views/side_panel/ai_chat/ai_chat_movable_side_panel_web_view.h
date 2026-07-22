// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_MOVABLE_SIDE_PANEL_WEB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_MOVABLE_SIDE_PANEL_WEB_VIEW_H_

#include <memory>

#include "components/web_modal/web_contents_modal_dialog_manager_delegate.h"
#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/controls/webview/webview.h"
#include "url/gurl.h"

namespace blink::mojom {
class FileChooserParams;
class WindowFeatures;
}  // namespace blink::mojom

namespace content {
class FileSelectListener;
class NavigationHandle;
class RenderFrameHost;
class WebContents;
struct OpenURLParams;
}  // namespace content

namespace input {
struct NativeWebKeyboardEvent;
}  // namespace input

namespace web_modal {
class WebContentsModalDialogHost;
}  // namespace web_modal

class Profile;
class SidePanelEntryScope;
class StatusBubbleViews;

// A side panel host for AI Chat used when the
// `kAIChatMoveFullPageToSidePanel` feature is enabled. Unlike the WebUI-wrapper
// based `AIChatSidePanelWebView`, this is a plain `views::WebView` that owns
// its AI Chat `WebContents` directly (modeled on Chromium's Contextual Tasks
// `ContextualTasksWebView`). Owning the contents allows the same live
// `WebContents` to be moved between a browser tab and the side panel without
// re-navigating.
class AIChatMovableSidePanelWebView
    : public views::WebView,
      public web_modal::WebContentsModalDialogManagerDelegate {
 public:
  // Factory used by `AIChatSidePanelWebView::CreateView` when the feature is
  // enabled. If `is_tab_associated` is true the panel tracks the active tab's
  // conversation; otherwise it opens the standalone (global) conversation.
  static std::unique_ptr<views::View> CreateView(Profile* profile,
                                                 bool is_tab_associated,
                                                 SidePanelEntryScope& scope);

  explicit AIChatMovableSidePanelWebView(Profile* profile);
  ~AIChatMovableSidePanelWebView() override;

  AIChatMovableSidePanelWebView(const AIChatMovableSidePanelWebView&) = delete;
  AIChatMovableSidePanelWebView& operator=(
      const AIChatMovableSidePanelWebView&) = delete;

  // Takes ownership of `web_contents` and displays it in this view.
  void AdoptWebContents(std::unique_ptr<content::WebContents> web_contents);

  // views::WebView:
  void SetWebContents(content::WebContents* web_contents) override;

  // views::View:
  // Keep the status bubble anchored to the bottom-left of the panel as it
  // resizes, mirroring how `AIChatSidePanelWebView` drives its status bubble.
  bool GetNeedsNotificationWhenVisibleBoundsChange() const override;
  void OnVisibleBoundsChanged() override;

  // content::WebContentsDelegate:
  // The side panel's `WebContents` delegate does not drive the browser status
  // bubble, so hovering a link would otherwise disclose nothing. Show the
  // hovered URL in our own status bubble, mirroring a normal tab. Keeping this
  // parity with `AIChatSidePanelWebView` ensures every clickable link discloses
  // its destination even when this movable panel becomes the default.
  void UpdateTargetURL(content::WebContents* source, const GURL& url) override;
  content::WebContents* AddNewContents(
      content::WebContents* source,
      std::unique_ptr<content::WebContents> new_contents,
      const GURL& target_url,
      WindowOpenDisposition disposition,
      const blink::mojom::WindowFeatures& window_features,
      bool user_gesture,
      bool* was_blocked) override;
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params,
      base::OnceCallback<void(content::NavigationHandle&)>
          navigation_handle_callback) override;
  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      scoped_refptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params) override;
  bool HandleKeyboardEvent(content::WebContents* source,
                           const input::NativeWebKeyboardEvent& event) override;

  // web_modal::WebContentsModalDialogManagerDelegate:
  web_modal::WebContentsModalDialogHost* GetWebContentsModalDialogHost(
      content::WebContents* web_contents) override;

  // Returns the most recent hovered-link URL forwarded to the status bubble
  // (empty when no link is hovered). Lets tests verify that link-hover
  // destination disclosure is wired up.
  const GURL& status_bubble_url_for_testing() const {
    return status_bubble_url_for_testing_;
  }

 private:
  // Attach a modal dialog manager to `web_contents` so dialogs display
  // correctly while it is hosted in the side panel.
  void AttachWebContentsModalDialogManager(content::WebContents* web_contents);

  // Detach the modal dialog manager from `web_contents` when it leaves the
  // side panel.
  void DetachWebContentsModalDialogManager(content::WebContents* web_contents);

  // The AI Chat `WebContents` owned and displayed by this view.
  std::unique_ptr<content::WebContents> owned_web_contents_;

  // Handles keyboard events that come back unhandled from the renderer.
  views::UnhandledKeyboardEventHandler unhandled_keyboard_event_handler_;

  // Shows the hovered link's URL in the bottom-left of the panel, like a tab.
  std::unique_ptr<StatusBubbleViews> status_bubble_;

  // Mirror of the last URL forwarded to `status_bubble_`, for tests.
  GURL status_bubble_url_for_testing_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_AI_CHAT_AI_CHAT_MOVABLE_SIDE_PANEL_WEB_VIEW_H_
