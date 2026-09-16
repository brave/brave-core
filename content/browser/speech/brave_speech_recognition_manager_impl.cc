// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/content/browser/speech/brave_speech_recognition_manager_impl.h"

#include <memory>

#include "base/feature_list.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"
#include "content/browser/speech/on_device_speech_recognition_engine_impl.h"

namespace content {

// Whether to use Brave's engine for on-device speech recognition sessions.
bool UsesBraveOnDeviceSpeechEngine() {
  return base::FeatureList::IsEnabled(
      local_ai::kBraveOnDeviceSpeechRecognition);
}

// Builds the engine for the on-device branch.
std::unique_ptr<SpeechRecognitionEngine> MakeOnDeviceSpeechEngine(
    const SpeechRecognitionSessionConfig& config) {
  if (UsesBraveOnDeviceSpeechEngine()) {
    return std::make_unique<BraveOnDeviceSpeechRecognitionEngine>(config);
  }
  return std::make_unique<OnDeviceSpeechRecognitionEngine>(config);
}

}  // namespace content
