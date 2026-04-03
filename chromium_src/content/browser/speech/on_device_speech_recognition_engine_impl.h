/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_IMPL_H_

namespace content {
class BraveSpeechRecognitionEngine;
}

// Inject friend so our subclass can access private
// members for session creation and stop flow.
#define CreateModelClientOnUI(...)           \
  CreateModelClientOnUI_Unused(__VA_ARGS__); \
  friend class BraveSpeechRecognitionEngine; \
  void CreateModelClientOnUI(__VA_ARGS__)

#include <content/browser/speech/on_device_speech_recognition_engine_impl.h>  // IWYU pragma: export

#undef CreateModelClientOnUI

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_ENGINE_IMPL_H_
