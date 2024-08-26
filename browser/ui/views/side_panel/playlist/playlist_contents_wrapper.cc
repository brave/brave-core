/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/playlist/playlist_contents_wrapper.h"

#include <utility>

#include "brave/browser/ui/views/side_panel/playlist/playlist_side_panel_coordinator.h"
#include "chrome/browser/picture_in_picture/picture_in_picture_window_manager.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_within_tab_helper.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "third_party/blink/public/mojom/frame/fullscreen.mojom.h"
#include "ui/views/widget/widget.h"

PlaylistContentsWrapper::PlaylistContentsWrapper(
    const GURL& webui_url,
    Profile* profile,
    int task_manager_string_id,
    bool esc_closes_ui,
    BrowserView* browser_view,
    PlaylistSidePanelCoordinator* coordinator)
    : WebUIContentsWrapperT(webui_url,
                            profile,
                            task_manager_string_id,
                            esc_closes_ui),
      browser_view_(browser_view),
      coordinator_(coordinator) {}

PlaylistContentsWrapper::~PlaylistContentsWrapper() = default;

bool PlaylistContentsWrapper::CanEnterFullscreenModeForTab(
    content::RenderFrameHost* requesting_frame) {
  return true;
}

void PlaylistContentsWrapper::EnterFullscreenModeForTab(
    content::RenderFrameHost* requesting_frame,
    const blink::mojom::FullscreenOptions& options) {
  FullscreenWithinTabHelper::CreateForWebContents(web_contents());
  FullscreenWithinTabHelper::FromWebContents(web_contents())
      ->SetIsFullscreenWithinTab(true);

  auto* fullscreen_controller = browser_view_->browser()
                                    ->exclusive_access_manager()
                                    ->fullscreen_controller();
  was_browser_fullscreen_ = fullscreen_controller->IsFullscreenForBrowser();
  DCHECK(!fullscreen_controller->IsTabFullscreen())
      << "We don't expect this case. In tab fullscreen, sidebar is not "
         "visible.";

  auto* widget = browser_view_->GetWidget();
  DCHECK(widget);
  fullscreen_display_id_ = options.display_id;
  if (was_browser_fullscreen_) {
    // In case it was in fullscreen for browser, we should trigger layout here.
    auto side_panel_web_view = coordinator_->side_panel_web_view();
    DCHECK(side_panel_web_view);
    side_panel_web_view->InvalidateLayout();
  } else {
    widget->SetFullscreen(true, fullscreen_display_id_);
  }

  fullscreen_observation_.Observe(fullscreen_controller);
}

void PlaylistContentsWrapper::ExitFullscreenModeForTab(
    content::WebContents* contents) {
  // The exit request from renderer.
  DCHECK(IsFullscreenForPlaylist());

  auto* widget = browser_view_->GetWidget();
  DCHECK(widget);
  if (was_browser_fullscreen_) {
    OnExitFullscreen();
  } else {
    widget->SetFullscreen(false);
    // Other clean-ups will be done OnExitFullscreen() when it's triggered by
    // fullscreen controller.
  }
}

void PlaylistContentsWrapper::OnFullscreenStateChanged() {
  // There're two known ways where this is triggered
  //  * press fullscreen button on the web page -> ExitFullscreenModeForTab()
  //    will be invoked by the renderer.
  //  * press shortcut key, such as Fn + f or F11 -> The browser will handle
  //    shortcut  and this will be invoked.
  // TODO(sko) When shortcut was pressed, we can't determine if we should go
  // back to fullscreen for browser, as the browser already has exited
  // fullscreen by itself. We might need more customization in BrowserView or
  // FullscreenController.
  auto* widget = browser_view_->GetWidget();
  DCHECK(widget);

  if (!widget->IsFullscreen() && IsFullscreenForPlaylist()) {
    DVLOG(2) << __FUNCTION__ << " Will exit fullscreen";
    OnExitFullscreen();
  }
}

bool PlaylistContentsWrapper::IsFullscreenForTabOrPending(
    const content::WebContents* web_contents) {
  return IsFullscreenForPlaylist();
}

content::FullscreenState PlaylistContentsWrapper::GetFullscreenState(
    const content::WebContents* web_contents) const {
  if (IsFullscreenForPlaylist()) {
    return {.target_mode = content::FullscreenMode::kContent,
            .target_display_id = fullscreen_display_id_};
  }

  return {};
}

content::PictureInPictureResult PlaylistContentsWrapper::EnterPictureInPicture(
    content::WebContents* web_contents) {
  return PictureInPictureWindowManager::GetInstance()
      ->EnterVideoPictureInPicture(web_contents);
}

void PlaylistContentsWrapper::ExitPictureInPicture() {
  PictureInPictureWindowManager::GetInstance()->ExitPictureInPicture();
}

content::WebContents* PlaylistContentsWrapper::AddNewContents(
    content::WebContents* source,
    std::unique_ptr<content::WebContents> new_contents,
    const GURL& target_url,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& window_features,
    bool user_gesture,
    bool* was_blocked) {
  return static_cast<WebContentsDelegate*>(browser_view_->browser())
      ->AddNewContents(source, std::move(new_contents), target_url, disposition,
                       window_features, user_gesture, was_blocked);
}

std::string PlaylistContentsWrapper::GetTitleForMediaControls(
    content::WebContents* web_contents) {
  // This string is DNT.
  return "Playlist";
}

bool PlaylistContentsWrapper::IsFullscreenForPlaylist() const {
  if (auto* fullscreen_tab_helper = FullscreenWithinTabHelper::FromWebContents(
          const_cast<PlaylistContentsWrapper*>(this)->web_contents())) {
    return fullscreen_tab_helper->is_fullscreen_within_tab();
  }

  return false;
}

void PlaylistContentsWrapper::OnExitFullscreen() {
  FullscreenWithinTabHelper::RemoveForWebContents(web_contents());
  fullscreen_observation_.Reset();
  fullscreen_display_id_ = display::kInvalidDisplayId;

  auto side_panel_web_view = coordinator_->side_panel_web_view();
  DCHECK(side_panel_web_view);
  side_panel_web_view->InvalidateLayout();
}
