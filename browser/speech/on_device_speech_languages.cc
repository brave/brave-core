/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speech/on_device_speech_languages.h"

#include <optional>
#include <string_view>

#include "base/feature_list.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "components/soda/constants.h"

namespace speech {

namespace {

// The locale that serves `language`, or null. Null for the multilingual
// model's locales while its feature is disabled.
const BraveOnDeviceSpeechLocale* GetServedLocale(std::string_view language) {
  const BraveOnDeviceSpeechLocale* locale =
      ResolveBraveOnDeviceSpeechLocale(language);
  if (!locale) {
    return nullptr;
  }
  if (locale->model == BraveOnDeviceSpeechModelKind::kMultilingual &&
      !base::FeatureList::IsEnabled(
          local_ai::kBraveOnDeviceSpeechMultilingual)) {
    return nullptr;
  }
  return locale;
}

}  // namespace

std::optional<std::string_view> GetOnDeviceSpeechLocale(
    std::string_view language) {
  const BraveOnDeviceSpeechLocale* locale = GetServedLocale(language);
  if (!locale) {
    return std::nullopt;
  }
  return locale->locale;
}

std::optional<local_ai::OnDeviceSpeechModel> GetOnDeviceSpeechModelForLanguage(
    std::string_view language) {
  const BraveOnDeviceSpeechLocale* locale = GetServedLocale(language);
  if (!locale) {
    return std::nullopt;
  }
  return locale->model == BraveOnDeviceSpeechModelKind::kEnglish
             ? local_ai::OnDeviceSpeechModel::kEnglish
             : local_ai::OnDeviceSpeechModel::kMultilingual;
}

}  // namespace speech
