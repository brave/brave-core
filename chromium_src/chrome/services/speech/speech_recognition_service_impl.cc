/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/services/speech/speech_recognition_service_impl.h"

#include "base/path_service.h"

namespace speech {

void SpeechRecognitionServiceImpl::SetAsrSession(
    mojo::PendingRemote<
        on_device_model::mojom::AsrStreamInput> stream,
    mojo::PendingReceiver<
        on_device_model::mojom::AsrStreamResponder>
        responder,
    SetAsrSessionCallback callback) {
  asr_stream_ = std::move(stream);
  asr_responder_ = std::move(responder);

  // Set dummy paths so FilePathsExist() returns true.
  // BraveSodaClient ignores the binary path.
  base::FilePath exe_path;
  base::PathService::Get(base::FILE_EXE, &exe_path);
  binary_path_ = exe_path;
  if (config_paths_.empty()) {
    config_paths_["en-US"] = exe_path;
    default_live_caption_language_ = "en-US";
  }

  std::move(callback).Run();
}

}  // namespace speech

#include <chrome/services/speech/speech_recognition_service_impl.cc>
