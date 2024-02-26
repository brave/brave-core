/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_RESPONDER_IMPL_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_RESPONDER_IMPL_H_

#include <variant>

#include "base/functional/callback_forward.h"
#include "base/values.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/render_frame_host_receiver_set.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"

class GURL;

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace playlist {

// `PlaylistMediaHandler` owns a set of Channel-associated
// `mojom::PlaylistMediaResponder` receivers, through which it can receive
// `OnMediaDetected` messages coming from remote `RenderFrame`s of a given
// `WebContents`.
// `mojom::PlaylistMediaResponder` is exposed to `RenderFrame`s in
// `BraveContentBrowserClient::RegisterAssociatedInterfaceBindersForRenderFrameHost()`.
class PlaylistMediaHandler final
    : public content::WebContentsUserData<PlaylistMediaHandler>,
      public mojom::PlaylistMediaResponder {
  using Signature = void(base::Value::List, const GURL&);
  using OnMediaDetectedCallback =
      std::variant<base::OnceCallback<Signature>,
                   base::RepeatingCallback<Signature>>;

 public:
  PlaylistMediaHandler(const PlaylistMediaHandler&) = delete;
  PlaylistMediaHandler& operator=(const PlaylistMediaHandler&) = delete;
  ~PlaylistMediaHandler() override;

  static void BindMediaResponderReceiver(
      content::RenderFrameHost* render_frame_host,
      mojo::PendingAssociatedReceiver<mojom::PlaylistMediaResponder> receiver);

 private:
  friend class content::WebContentsUserData<PlaylistMediaHandler>;

  // Depending on the callback you pass to
  // `PlaylistMediaHandler::CreateForWebContents()` when creating the
  // `PlaylistMediaHandler`, you either get back the first non-empty list of
  // media (`base::OnceCallback<>`/background `WebContents` case), or all the
  // non-empty lists of media (`base::RepeatingCallback<>`/regular tab case), as
  // long as the `WebContents` is alive.
  PlaylistMediaHandler(content::WebContents* web_contents,
                       OnMediaDetectedCallback on_media_detected_callback);

  // mojom::PlaylistMediaResponder:
  void OnMediaDetected(base::Value::List media) override;

  content::RenderFrameHostReceiverSet<mojom::PlaylistMediaResponder>
      media_responder_receivers_;
  OnMediaDetectedCallback on_media_detected_callback_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_RESPONDER_IMPL_H_
