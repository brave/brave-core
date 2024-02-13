/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_REALTIME_ANALYSER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_REALTIME_ANALYSER_H_

#include <optional>
#include <utility>

#include "brave/third_party/blink/renderer/platform/brave_audio_farbling_helper.h"

#define analysis_frame_                                 \
  analysis_frame_;                                      \
                                                        \
 public:                                                \
  void set_audio_farbling_helper(                       \
      std::optional<BraveAudioFarblingHelper> helper) { \
    audio_farbling_helper_ = std::move(helper);         \
  }                                                     \
                                                        \
 private:                                               \
  std::optional<BraveAudioFarblingHelper> audio_farbling_helper_

#include "src/third_party/blink/renderer/modules/webaudio/realtime_analyser.h"  // IWYU pragma: export

#undef analysis_frame_

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_REALTIME_ANALYSER_H_
