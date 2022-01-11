/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_SYNTHESIS_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_SYNTHESIS_H_

// We need to include this here before redefining OnSetVoiceList to avoid a
// name collision with a method in the mojom class.
#include "third_party/blink/public/mojom/speech/speech_synthesis.mojom-blink.h"

#define OnSetVoiceList                                       \
  OnSetVoiceList_ChromiumImpl(                               \
      Vector<mojom::blink::SpeechSynthesisVoicePtr> voices); \
  void OnSetVoiceList

#include "src/third_party/blink/renderer/modules/speech/speech_synthesis.h"
#undef OnSetVoiceList

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_SPEECH_SPEECH_SYNTHESIS_H_
