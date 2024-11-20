// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CONTENT_YOUTUBE_TAB_HELPER_H_
#define BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CONTENT_YOUTUBE_TAB_HELPER_H_

#include <string>

#include "base/component_export.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_json.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "build/build_config.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace youtube_script_injector {

class YouTubeRegistry;

// Used to inject JS scripts into the page.
class COMPONENT_EXPORT(YOUTUBE_SCRIPT_INJECTOR_BROWSER_CONTENT) YouTubeTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<YouTubeTabHelper> {
 public:
  static void MaybeCreateForWebContents(content::WebContents* contents,
                                        const int32_t world_id);
  ~YouTubeTabHelper() override;
  YouTubeTabHelper(const YouTubeTabHelper&) = delete;
  YouTubeTabHelper& operator=(const YouTubeTabHelper&) = delete;

 private:
  YouTubeTabHelper(content::WebContents*, const int32_t world_id);
  // Called to insert the YouTube script into the page.
  void InsertScriptInPage(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      MatchedRule rule);
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>& GetRemote(
      content::RenderFrameHost* rfh);
  friend class content::WebContentsUserData<YouTubeTabHelper>;

  // content::WebContentsObserver overrides
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  const int32_t world_id_;
  const raw_ptr<YouTubeRegistry> youtube_registry_;  // NOT OWNED
  // The remote used to send the script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
  base::WeakPtrFactory<YouTubeTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace youtube_script_injector

#endif  // BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CONTENT_YOUTUBE_TAB_HELPER_H_
