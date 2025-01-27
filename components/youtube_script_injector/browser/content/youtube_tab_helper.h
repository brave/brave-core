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
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_json.h"
#include "build/build_config.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace youtube_script_injector {

class YouTubeRegistry;

// Used to inject JS scripts into the page.
class COMPONENT_EXPORT(YOUTUBE_SCRIPT_INJECTOR_BROWSER_CONTENT) YouTubeTabHelper final
    : public content::WebContentsObserver,
      public content::WebContentsUserData<YouTubeTabHelper> {
 public:
  static void MaybeCreateForWebContents(content::WebContents* contents,
                                        const int32_t world_id);
  static void EnterPipMode();
  ~YouTubeTabHelper() override;
  YouTubeTabHelper(const YouTubeTabHelper&) = delete;
  YouTubeTabHelper& operator=(const YouTubeTabHelper&) = delete;

  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>& GetRemote(
      content::RenderFrameHost* rfh);

  int32_t GetWorldId() const { return world_id_; }
  bool IsMediaPlaying() const { return is_media_playing_; }
  const std::optional<YouTubeJson>& GetJson() const;
  // Called to insert a YouTube script into the page.
  void InsertScriptInPage(
      const content::GlobalRenderFrameHostId& render_frame_host_id,
      blink::mojom::UserActivationOption activation,
      std::string script);
  base::WeakPtr<YouTubeTabHelper> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  YouTubeTabHelper(content::WebContents*, const int32_t world_id);
  friend class content::WebContentsUserData<YouTubeTabHelper>;

  // content::WebContentsObserver overrides

  // This method is invoked once the window.document element of the primary main
  // frame's current document (i.e., |render_frame_host|) is ready. This happens
  // when the document's main HTML resource has finished parsing. Here
  // document element refers to DOMDocument, which is different from browser
  // implementation of blink::Document in DocumentUserData/DocumentService which
  // are typically created when navigation commits.
  //
  // Note that PrimaryMainDocumentElementAvailable should be used when the
  // observers which send IPCs to the renderer want to ensure that
  // window.document is non-null. For for the comment cases like observing
  // primary document/URL changes in the omnibox due to navigation
  // WebContentsObserver::PrimaryPageChanged should be used and to observe fully
  // loaded signal WebContentsObserver::DidFinishLoad can be used.
  //
  // This event is dispatched once in the document's lifetime, which means it's
  // not dispatched after navigation that restores a Back/Forward Cache page.
  // For prerendering, this signal is dispatched when the main document element
  // is available and the document is shown to the user (i.e., after the
  // activation).
  void PrimaryMainDocumentElementAvailable() override;

  void MediaStartedPlaying(const MediaPlayerInfo& video_type,
                           const content::MediaPlayerId& id) override;
  void MediaStoppedPlaying(
      const MediaPlayerInfo& video_type,
      const content::MediaPlayerId& id,
      WebContentsObserver::MediaStoppedReason reason) override;

  const int32_t world_id_;
  const raw_ptr<YouTubeRegistry> youtube_registry_;  // NOT OWNED
  // The remote used to send the script to the renderer.
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
  bool is_media_playing_ = false;
  base::WeakPtrFactory<YouTubeTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace youtube_script_injector

#endif  // BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CONTENT_YOUTUBE_TAB_HELPER_H_
