/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_CONTENTS_WRAPPER_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_CONTENTS_WRAPPER_H_

#include <memory>
#include <string>

#include "base/scoped_observation.h"
#include "brave/browser/ui/webui/playlist_ui.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_observer.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"

class BrowserView;
class PlaylistSidePanelCoordinator;
class FullscreenController;

// Implements WebContentsDelegate parts for Playlist features.
class PlaylistContentsWrapper
    : public WebUIContentsWrapperT<playlist::PlaylistUI>,
      public FullscreenObserver {
 public:
  PlaylistContentsWrapper(const GURL& webui_url,
                          Profile* profile,
                          int task_manager_string_id,
                          bool esc_closes_ui,
                          BrowserView* browser_view,
                          PlaylistSidePanelCoordinator* coordinator);
  ~PlaylistContentsWrapper() override;

  // WebUIContentsWrapperT<playlist::PlaylistUI>:
  bool CanEnterFullscreenModeForTab(
      content::RenderFrameHost* requesting_frame) override;
  void EnterFullscreenModeForTab(
      content::RenderFrameHost* requesting_frame,
      const blink::mojom::FullscreenOptions& options) override;
  void ExitFullscreenModeForTab(content::WebContents* contents) override;

  bool IsFullscreenForTabOrPending(
      const content::WebContents* web_contents) override;
  content::FullscreenState GetFullscreenState(
      const content::WebContents* web_contents) const override;

  content::PictureInPictureResult EnterPictureInPicture(
      content::WebContents* web_contents) override;
  void ExitPictureInPicture() override;

  void AddNewContents(content::WebContents* source,
                      std::unique_ptr<content::WebContents> new_contents,
                      const GURL& target_url,
                      WindowOpenDisposition disposition,
                      const blink::mojom::WindowFeatures& window_features,
                      bool user_gesture,
                      bool* was_blocked) override;

  std::string GetTitleForMediaControls(
      content::WebContents* web_contents) override;

  // FullscreenObserver:
  void OnFullscreenStateChanged() override;

 private:
  bool IsFullscreenForPlaylist() const;

  void OnExitFullscreen();

  raw_ptr<BrowserView> browser_view_ = nullptr;
  raw_ptr<PlaylistSidePanelCoordinator> coordinator_ = nullptr;

  bool was_browser_fullscreen_ = false;
  int64_t fullscreen_display_id_ = display::kInvalidDisplayId;

  base::ScopedObservation<FullscreenController, FullscreenObserver>
      fullscreen_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_PLAYLIST_PLAYLIST_CONTENTS_WRAPPER_H_
