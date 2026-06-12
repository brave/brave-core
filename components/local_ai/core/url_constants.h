// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_URL_CONSTANTS_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_URL_CONSTANTS_H_

namespace local_ai {

inline constexpr char kUntrustedLocalAIHost[] = "local-ai";
inline constexpr char kUntrustedLocalAIURL[] = "chrome-untrusted://local-ai/";

// Cross-origin-isolated worker that hosts the onnxruntime-web (ORT-Web)
// Nemotron 0.6B on-device speech-recognition backend. Kept on its own origin
// so the COOP/COEP isolation ORT-Web needs for multi-threaded
// WASM is contained to this worker.
inline constexpr char kOnDeviceSpeechRecognitionWorkerHost[] =
    "on-device-speech-recognition-worker";
inline constexpr char kOnDeviceSpeechRecognitionWorkerURL[] =
    "chrome-untrusted://on-device-speech-recognition-worker/";

// Trusted WebUI dev/QA test page that drives the on-device speech
// recognition pipeline end to end for verification.
inline constexpr char kOnDeviceSpeechRecognitionInternalsHost[] =
    "on-device-speech-recognition-internals";
inline constexpr char kOnDeviceSpeechRecognitionInternalsURL[] =
    "chrome://on-device-speech-recognition-internals/";

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_URL_CONSTANTS_H_
