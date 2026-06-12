/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_IMPL_H_

// Include, ahead of the macro below, every header that declares an unrelated
// `Available(...)` method so their guards are set and the macro can't rewrite
// them: the base mojom interface (media::mojom::OnDeviceSpeechRecognition) and
// the optimization-guide model broker (model_broker.mojom / model_broker_client
// each declare their own `Available`). Only the override declared in the impl
// header below is rewritten.
#include "build/build_config.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"

#if !BUILDFLAG(IS_ANDROID)
#include "components/optimization_guide/core/model_execution/model_broker_client.h"  // nogncheck crbug.com/40147906
#include "components/optimization_guide/core/model_execution/remote_model_executor.h"  // nogncheck crbug.com/40147906
#include "components/soda/soda_installer.h"  // nogncheck crbug.com/40147906
#endif                                       // !BUILDFLAG(IS_ANDROID)

// Brave ships its own on-device speech recognition model via the component
// updater, so the SODA download-state gating in the upstream `Available` does
// not apply. Preserve the upstream implementation as `Available_ChromiumImpl`
// and add Brave's own `Available` override (defined in the .cc). The real
// override is declared last so the trailing `override` keyword binds to it.
#define Available(...)                 \
  Available_ChromiumImpl(__VA_ARGS__); \
  void Available(__VA_ARGS__)

#include <chrome/browser/speech/on_device_speech_recognition_impl.h>  // IWYU pragma: export

#undef Available

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_ON_DEVICE_SPEECH_RECOGNITION_IMPL_H_
