/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_CONTEXT_H_

#include "media/mojo/mojom/media_player.mojom-blink.h"

// A stub to `RequestFullscreen` which is being inserted as a mojom extension to
// `MediaPlayer`. See: chromium_src/media/mojo/mojom/media_player.mojom. This
// extension doesn't seem to be applicable to `AudioContext`.
#define RequestEnterPictureInPicture \
  RequestFullscreen() override {}    \
  void RequestEnterPictureInPicture

#include <third_party/blink/renderer/modules/webaudio/audio_context.h>  // IWYU pragma: export

#undef RequestEnterPictureInPicture

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_CONTEXT_H_
