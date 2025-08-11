/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_YOUTUBE_SCRIPT_INJECTOR_TAB_HELPER_H_
#define BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_YOUTUBE_SCRIPT_INJECTOR_TAB_HELPER_H_

#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "base/values.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

class YouTubeScriptInjectorTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<YouTubeScriptInjectorTabHelper> {
 public:
  explicit YouTubeScriptInjectorTabHelper(content::WebContents* contents);
  YouTubeScriptInjectorTabHelper(const YouTubeScriptInjectorTabHelper&) =
      delete;
  YouTubeScriptInjectorTabHelper& operator=(
      const YouTubeScriptInjectorTabHelper&) = delete;
  ~YouTubeScriptInjectorTabHelper() override;
  bool IsYouTubeVideo(bool mobileOnly = false) const;
  void MaybeSetFullscreen();

  // Fullscreen state management using PageUserData
  bool HasFullscreenBeenRequested() const;
  void SetFullscreenRequested(bool requested);

  // Check if Picture-in-Picture is available for the current page.
  bool IsPictureInPictureAvailable() const;

  // content::WebContentsObserver overrides:
  void PrimaryPageChanged(content::Page& page) override;
  void RenderFrameDeleted(content::RenderFrameHost* rfh) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void PrimaryMainDocumentElementAvailable() override;
  void MediaEffectivelyFullscreenChanged(bool is_fullscreen) override;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

 private:
  // Callback for when the fullscreen script completes.
  void OnFullscreenScriptComplete(content::GlobalRenderFrameHostToken token,
                                  base::Value value);

  void EnsureBound(content::RenderFrameHost* rfh);

  // The remote used to send the fullscreen script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
  content::GlobalRenderFrameHostId bound_rfh_id_;

  base::WeakPtrFactory<YouTubeScriptInjectorTabHelper> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_YOUTUBE_SCRIPT_INJECTOR_TAB_HELPER_H_
