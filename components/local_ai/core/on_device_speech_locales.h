/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_LOCALES_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_LOCALES_H_

#include <string_view>

#include "base/containers/span.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"

namespace local_ai {

// A locale that Brave's on-device speech recognition serves.
struct OnDeviceSpeechLocale {
  // The canonical BCP 47 tag. The speech worker gets it as the language of a
  // stream.
  std::string_view tag;
  // The model that transcribes the locale. It does not tell whether Brave can
  // offer the model.
  OnDeviceSpeechModel model = OnDeviceSpeechModel::kEnglish;
};

// All the locales that Brave's on-device speech recognition serves.
base::span<const OnDeviceSpeechLocale> GetOnDeviceSpeechLocales();

// The locale that serves `language`, a page's BCP 47 tag, or null when no
// locale does. For example, "de-AT" gives "de-DE".
const OnDeviceSpeechLocale* ResolveOnDeviceSpeechLocale(
    std::string_view language);

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_ON_DEVICE_SPEECH_LOCALES_H_
