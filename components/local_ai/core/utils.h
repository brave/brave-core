/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_UTILS_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_UTILS_H_

#include <optional>

#include "mojo/public/cpp/base/big_buffer.h"

class PrefService;

namespace base {
class FilePath;
}  // namespace base

namespace local_ai {

enum class OnDeviceSpeechModel;
struct OnDeviceSpeechLocale;

// Whether Brave's feature and the Local AI master switch both allow on-device
// speech recognition. A null `local_state` reads as the switch's default, on.
bool CanOfferOnDeviceSpeech(const PrefService* local_state);

// Whether Brave can offer `model`. True when `CanOfferOnDeviceSpeech` is true
// and, for the multilingual model, `kBraveOnDeviceSpeechMultilingual` is
// enabled. A null `local_state` reads as the switch's default, on.
bool CanOfferOnDeviceSpeechModel(const PrefService* local_state,
                                 OnDeviceSpeechModel model);

// The model that the user enabled (`prefs::kOnDeviceSpeechModel`), or
// std::nullopt when the user enabled none. A null `local_state` reads as none.
std::optional<OnDeviceSpeechModel> GetEnabledOnDeviceSpeechModel(
    const PrefService* local_state);

// Enables `model`, in place of the model that the user enabled before.
void EnableOnDeviceSpeechModel(PrefService& local_state,
                               OnDeviceSpeechModel model);

// Whether `model` is to be on disk. True when `CanOfferOnDeviceSpeechModel` is
// true for `model` and the user enabled `model`. A null `local_state` reads as
// not enabled.
bool ShouldInstallOnDeviceSpeechModel(const PrefService* local_state,
                                      OnDeviceSpeechModel model);

// The model on disk that transcribes `locale`, or std::nullopt when none is.
// Only a model that Brave can offer counts. The multilingual model transcribes
// every locale, so it comes first when both models are on disk.
std::optional<OnDeviceSpeechModel> GetInstalledOnDeviceSpeechModel(
    const PrefService* local_state,
    const OnDeviceSpeechLocale& locale);

// Reads a file directly into BigBuffer storage. For files >64KB BigBuffer
// uses shared memory. Returns std::nullopt if the file can't be opened, is
// empty, or can't be fully read. Must run on a sequence that allows blocking.
std::optional<mojo_base::BigBuffer> ReadFileToBigBuffer(
    const base::FilePath& path);

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_UTILS_H_
