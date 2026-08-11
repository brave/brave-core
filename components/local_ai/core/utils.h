/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_UTILS_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_UTILS_H_

#include <optional>

#include "base/component_export.h"
#include "media/mojo/mojom/speech_recognizer.mojom.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace base {
class FilePath;
}  // namespace base

namespace local_ai {

// Reads a file directly into BigBuffer storage. For files >64KB BigBuffer
// uses shared memory. Returns std::nullopt if the file can't be opened, is
// empty, or can't be fully read. Must run on a sequence that allows blocking.
COMPONENT_EXPORT(LOCAL_AI_UTILS)
std::optional<mojo_base::BigBuffer> ReadFileToBigBuffer(
    const base::FilePath& path);

// Whether Brave's on-device speech recognition serves `quality`. Always false
// while the feature flag is off, since Brave then serves no quality at all.
COMPONENT_EXPORT(LOCAL_AI_UTILS)
bool IsQualityServedByBraveOnDeviceSpeech(
    media::mojom::SpeechRecognitionQuality quality);

}  // namespace local_ai

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_UTILS_H_
