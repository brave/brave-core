/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/local_ai/buildflags/buildflags.h"
#include "build/build_config.h"
#include "content/public/browser/speech_recognition_session_config.h"

// Hooks are defined in brave/content/browser/speech/
// brave_speech_recognition_manager_impl.cc, which is compiled into
// //content/browser through brave_content_browser_sources.
#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/content/browser/speech/brave_speech_recognition_manager_impl.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "content/browser/speech/on_device_speech_recognition_engine_impl.h"
#include "content/browser/speech/speech_recognition_engine.h"
#endif

#if !BUILDFLAG(ENABLE_LOCAL_AI)

namespace content {

bool UsesBraveOnDeviceSpeechEngine() {
  return false;
}

#if !BUILDFLAG(IS_ANDROID)
std::unique_ptr<SpeechRecognitionEngine> MakeOnDeviceSpeechEngine(
    const SpeechRecognitionSessionConfig& config) {
  return std::make_unique<OnDeviceSpeechRecognitionEngine>(config);
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace content

#endif  // !BUILDFLAG(ENABLE_LOCAL_AI)

#include <content/browser/speech/speech_recognition_manager_impl.cc>
