/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_YOUTUBE_SCRIPT_INJECTOR_TAB_HELPER_H_
#define BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_YOUTUBE_SCRIPT_INJECTOR_TAB_HELPER_H_

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

// Enters fullscreen and Picture-in-Picture for YouTube videos. Owned by
// BraveTabFeatures.
class YouTubeScriptInjectorTabHelper
    : public tabs::ContentsObservingTabFeature {
 public:
  DECLARE_USER_DATA(YouTubeScriptInjectorTabHelper);

  static YouTubeScriptInjectorTabHelper* From(tabs::TabInterface* tab);

  explicit YouTubeScriptInjectorTabHelper(tabs::TabInterface& tab);
  YouTubeScriptInjectorTabHelper(const YouTubeScriptInjectorTabHelper&) =
      delete;
  YouTubeScriptInjectorTabHelper& operator=(
      const YouTubeScriptInjectorTabHelper&) = delete;
  ~YouTubeScriptInjectorTabHelper() override;
  bool IsYouTubeDomain(bool mobileOnly = false) const;
  bool IsYouTubeVideo(bool mobileOnly = false) const;

  // Entry point for the PiP button: injects the fullscreen script and, on the
  // back of the resulting fullscreen transition, enters Picture-in-Picture.
  void MaybeSetFullScreenAndPictureInPictureMode();

  // Check if Picture-in-Picture is available for the current page.
  bool IsPictureInPictureAvailable() const;

  // content::WebContentsObserver overrides:
  void PrimaryPageChanged(content::Page& page) override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* rfh) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void PrimaryMainDocumentElementAvailable() override;
  void MediaEffectivelyFullscreenChanged(bool is_fullscreen) override;
  void MediaPictureInPictureChanged(bool is_picture_in_picture) override;

 private:
  // Callback for when the fullscreen script completes.
  void OnFullscreenScriptComplete(base::Value value);

  // Enters Picture-in-Picture for the page currently showing if a request is
  // still armed for it, consuming the request.
  void MaybeEnterPictureInPicture();

  void EnsureBound(content::RenderFrameHost* rfh);

  // The remote used to send the fullscreen script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;

  ui::ScopedUnownedUserData<YouTubeScriptInjectorTabHelper>
      scoped_unowned_user_data_;

  base::WeakPtrFactory<YouTubeScriptInjectorTabHelper> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_YOUTUBE_SCRIPT_INJECTOR_TAB_HELPER_H_
