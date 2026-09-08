// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Brave's half of chromium_src/content/browser/speech/
// speech_recognition_manager_impl.cc, which declares both of these and calls
// them, so that override needs no Brave include of its own. This file is
// compiled into //content/browser through brave_content_browser_sources.

#include <memory>

#include "base/feature_list.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"
#include "content/browser/speech/on_device_speech_recognition_engine_impl.h"
#include "content/browser/speech/speech_recognition_engine.h"
#include "content/common/content_export.h"
#include "content/public/browser/speech_recognition_session_config.h"

namespace content {

// Whether to use Brave's engine for on-device speech recognition sessions.
CONTENT_EXPORT bool UsesBraveOnDeviceSpeechEngine() {
  return base::FeatureList::IsEnabled(
      local_ai::kBraveOnDeviceSpeechRecognition);
}

// Builds the engine for the on-device branch.
CONTENT_EXPORT std::unique_ptr<SpeechRecognitionEngine>
MakeOnDeviceSpeechEngine(const SpeechRecognitionSessionConfig& config) {
  if (UsesBraveOnDeviceSpeechEngine()) {
    return std::make_unique<BraveOnDeviceSpeechRecognitionEngine>(config);
  }
  return std::make_unique<OnDeviceSpeechRecognitionEngine>(config);
}

}  // namespace content
