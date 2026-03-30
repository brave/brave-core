/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_SERVICES_SPEECH_SPEECH_RECOGNITION_SERVICE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_SERVICES_SPEECH_SPEECH_RECOGNITION_SERVICE_IMPL_H_

#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

#define FilePathsExist()                             \
  FilePathsExist_Unused();                           \
  void SetAsrSession(                                \
      mojo::PendingRemote<                           \
          on_device_model::mojom::AsrStreamInput>    \
          stream,                                    \
      mojo::PendingReceiver<                         \
          on_device_model::mojom::AsrStreamResponder>\
          responder,                                 \
      SetAsrSessionCallback callback) override;      \
 public:                                             \
  mojo::PendingRemote<                               \
      on_device_model::mojom::AsrStreamInput>        \
  TakeAsrStream() {                                  \
    return std::move(asr_stream_);                   \
  }                                                  \
  mojo::PendingReceiver<                             \
      on_device_model::mojom::AsrStreamResponder>    \
  TakeAsrResponder() {                               \
    return std::move(asr_responder_);                \
  }                                                  \
 protected:                                          \
  bool FilePathsExist()

#define mask_offensive_words_                         \
  mask_offensive_words_ = false;                      \
  mojo::PendingRemote<                                \
      on_device_model::mojom::AsrStreamInput>         \
      asr_stream_;                                    \
  mojo::PendingReceiver<                              \
      on_device_model::mojom::AsrStreamResponder>     \
      asr_responder_;                                 \
  [[maybe_unused]] bool mask_offensive_words_u_

#include <chrome/services/speech/speech_recognition_service_impl.h>  // IWYU pragma: export

#undef mask_offensive_words_
#undef FilePathsExist

#endif  // BRAVE_CHROMIUM_SRC_CHROME_SERVICES_SPEECH_SPEECH_RECOGNITION_SERVICE_IMPL_H_
