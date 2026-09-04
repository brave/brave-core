/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/feature_list.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "build/build_config.h"
#include "content/browser/speech/on_device_speech_recognition_engine_impl.h"
#include "content/browser/speech/speech_recognition_engine.h"
#include "content/public/browser/speech_recognition_session_config.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/local_ai/core/features.h"
#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"
#endif

namespace {

// Whether to use Brave's engine for on-device speech recognition sessions.
bool UsesBraveOnDeviceSpeechEngine() {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  return base::FeatureList::IsEnabled(
      local_ai::kBraveOnDeviceSpeechRecognition);
#else
  return false;
#endif
}

// Builds the engine for the on-device branch. Guarded to match the upstream
// call site, which sits inside `#if !BUILDFLAG(IS_ANDROID)`.
#if !BUILDFLAG(IS_ANDROID)
std::unique_ptr<content::SpeechRecognitionEngine> MakeOnDeviceSpeechEngine(
    const content::SpeechRecognitionSessionConfig& config) {
#if BUILDFLAG(ENABLE_LOCAL_AI)
  if (UsesBraveOnDeviceSpeechEngine()) {
    return std::make_unique<content::BraveOnDeviceSpeechRecognitionEngine>(
        config);
  }
#endif
  return std::make_unique<content::OnDeviceSpeechRecognitionEngine>(config);
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace

#include <content/browser/speech/speech_recognition_manager_impl.cc>
