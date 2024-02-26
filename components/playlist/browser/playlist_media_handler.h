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
#include "url/gurl.h"

namespace playlist {

class PlaylistMediaHandler final
    : public content::WebContentsUserData<PlaylistMediaHandler>,
      public mojom::PlaylistMediaResponder {
  using Signature = void(base::Value, const GURL&);
  using OnMediaDetectedCallback =
      std::variant<base::OnceCallback<Signature>,
                   base::RepeatingCallback<Signature>>;

 public:
  PlaylistMediaHandler(const PlaylistMediaHandler&) = delete;
  PlaylistMediaHandler& operator=(const PlaylistMediaHandler&) = delete;
  ~PlaylistMediaHandler() override;

  static void BindMediaResponderReceiver(
      content::RenderFrameHost* render_frame_host,
      mojo::PendingAssociatedReceiver<playlist::mojom::PlaylistMediaResponder>
          receiver);

 private:
  friend class content::WebContentsUserData<PlaylistMediaHandler>;

  PlaylistMediaHandler(content::WebContents* web_contents,
                       OnMediaDetectedCallback on_media_detected);

  void OnMediaDetected(base::Value media) override;

  content::RenderFrameHostReceiverSet<mojom::PlaylistMediaResponder>
      media_responder_receivers_;
  OnMediaDetectedCallback on_media_detected_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_RESPONDER_IMPL_H_
