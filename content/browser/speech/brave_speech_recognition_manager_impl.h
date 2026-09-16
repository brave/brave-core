// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_SPEECH_RECOGNITION_MANAGER_IMPL_H_
#define BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_SPEECH_RECOGNITION_MANAGER_IMPL_H_

#include <memory>

#include "content/browser/speech/speech_recognition_engine.h"
#include "content/public/browser/speech_recognition_session_config.h"

namespace content {

// Both are called by the plaster substitutions on
// content/browser/speech/speech_recognition_manager_impl.cc.
bool UsesBraveOnDeviceSpeechEngine();
std::unique_ptr<SpeechRecognitionEngine> MakeOnDeviceSpeechEngine(
    const SpeechRecognitionSessionConfig& config);

}  // namespace content

#endif  // BRAVE_CONTENT_BROWSER_SPEECH_BRAVE_SPEECH_RECOGNITION_MANAGER_IMPL_H_
