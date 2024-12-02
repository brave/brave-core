/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/browser/overlay_window.h"
#include "third_party/blink/public/mojom/mediasession/media_session.mojom.h"

// Sets media position along with playback state.
// Note that |media_position_| could be incorrect(suspecting timing issue
// between service process), so we're getting more correct position from media
// session.
#define SetPlaybackState(PLAYBACK_STATE)                                  \
  SetPlaybackState(PLAYBACK_STATE);                                       \
  if (auto position = MediaSessionImpl::Get(web_contents())               \
                          ->GetMediaPositionFromNormalPlayerIfPossible(); \
      position) {                                                         \
    window_->SetMediaPosition(position);                                  \
  } else {                                                                \
    window_->SetMediaPosition(media_position_);                           \
  }

// Update seeker enabled state whenever actions are updated.
// Note that we allow seeking when media session is controllable referring to
// upstream code.
// https://source.chromium.org/chromium/chromium/src/+/main:content/browser/media/session/media_session_impl.cc;l=1687;drc=c686e8f4fd379312469fe018f5c390e9c8f20d0d
#define SetSkipAdButtonVisibility(SKIP_BUTTON_VISIBILITY)       \
  SetSkipAdButtonVisibility(SKIP_BUTTON_VISIBILITY);            \
  auto* session = MediaSessionImpl::Get(web_contents());        \
  media_session_action_seek_to_handled_ =                       \
      session->ShouldRouteAction(                               \
          media_session::mojom::MediaSessionAction::kSeekTo) || \
      session->IsControllable();                                \
  window_->SetSeekerEnabled(media_session_action_seek_to_handled_)

#include "src/content/browser/picture_in_picture/video_picture_in_picture_window_controller_impl.cc"

#undef SetSkipAdButtonVisibility
#undef SetPlaybackState

namespace content {

void VideoPictureInPictureWindowControllerImpl::RequestFullscreen() {
  DCHECK(active_session_);

  active_session_->GetMediaPlayerRemote()->RequestFullscreen();
  FocusInitiator();
}

}  // namespace content
