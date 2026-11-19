/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/core/utils.h"

#include <utility>

#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "brave/components/local_ai/core/features.h"
#include "brave/components/local_ai/core/on_device_speech_models_state.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "components/prefs/pref_service.h"

namespace local_ai {

bool CanOfferOnDeviceSpeech(const PrefService* local_state) {
  return base::FeatureList::IsEnabled(kBraveOnDeviceSpeechRecognition) &&
         (!local_state || local_state->GetBoolean(prefs::kBraveLocalAIEnabled));
}

std::optional<OnDeviceSpeechModel> GetEnabledOnDeviceSpeechModel(
    const PrefService* local_state) {
  if (!local_state) {
    return std::nullopt;
  }
  const auto model = static_cast<OnDeviceSpeechModel>(
      local_state->GetInteger(prefs::kOnDeviceSpeechModel));
  switch (model) {
    case OnDeviceSpeechModel::kEnglish:
    case OnDeviceSpeechModel::kMultilingual:
      return model;
  }
  // 0 for no model, or a value that no model has, such as one from a later
  // version.
  return std::nullopt;
}

void EnableOnDeviceSpeechModel(PrefService& local_state,
                               OnDeviceSpeechModel model) {
  local_state.SetInteger(prefs::kOnDeviceSpeechModel,
                         std::to_underlying(model));
}

bool ShouldInstallOnDeviceSpeechModel(const PrefService* local_state,
                                      OnDeviceSpeechModel model) {
  if (!local_state || !CanOfferOnDeviceSpeech(local_state)) {
    return false;
  }
  if (model == OnDeviceSpeechModel::kMultilingual &&
      !base::FeatureList::IsEnabled(kBraveOnDeviceSpeechMultilingual)) {
    return false;
  }
  return GetEnabledOnDeviceSpeechModel(local_state) == model;
}

std::optional<mojo_base::BigBuffer> ReadFileToBigBuffer(
    const base::FilePath& path) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    DVLOG(0) << "Failed to open: " << path;
    return std::nullopt;
  }
  int64_t size = file.GetLength();
  if (size <= 0) {
    DVLOG(0) << "Empty or unreadable: " << path;
    return std::nullopt;
  }
  mojo_base::BigBuffer buffer(static_cast<size_t>(size));
  if (!file.ReadAndCheck(0, base::span<uint8_t>(buffer))) {
    DVLOG(0) << "Failed to read: " << path;
    return std::nullopt;
  }
  return buffer;
}

}  // namespace local_ai
