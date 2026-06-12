/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_IMPL_H_

namespace content {
class BraveOnDeviceSpeechRecognitionEngine;
}

// Inject `friend class BraveOnDeviceSpeechRecognitionEngine;` into
// `OnDeviceSpeechRecognitionEngine` so our subclass can touch the
// base's private members (`config_`, `accumulated_audio_data_`,
// `asr_stream_`, `ConvertAccumulatedAudioData`, `main_sequence_checker_`)
// when driving Brave's controller-owned session.
// Anchored on a stable private method.
#define OnRecognizerDisconnected()                   \
  OnRecognizerDisconnected_Unused();                 \
  friend class BraveOnDeviceSpeechRecognitionEngine; \
  void OnRecognizerDisconnected()

#include <content/browser/speech/on_device_speech_recognition_engine_impl.h>  // IWYU pragma: export

#undef OnRecognizerDisconnected

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_IMPL_H_
