/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/services/speech/speech_recognition_recognizer_impl.h"

#include "brave/components/speech/brave_soda_client.h"

// Rename the class so all upstream method definitions
// (including Create) go on _ChromiumImpl.
#define SpeechRecognitionRecognizerImpl \
  SpeechRecognitionRecognizerImpl_ChromiumImpl

#include <chrome/services/speech/speech_recognition_recognizer_impl.cc>

#undef SpeechRecognitionRecognizerImpl

namespace speech {

// static
void SpeechRecognitionRecognizerImpl::Create(
    mojo::PendingReceiver<
        media::mojom::SpeechRecognitionRecognizer>
        receiver,
    mojo::PendingRemote<
        media::mojom::SpeechRecognitionRecognizerClient>
        remote,
    media::mojom::SpeechRecognitionOptionsPtr options,
    const base::FilePath& binary_path,
    const base::flat_map<std::string, base::FilePath>&
        config_paths,
    const std::string& primary_language_name,
    const bool mask_offensive_words,
    base::WeakPtr<SpeechRecognitionServiceImpl>
        speech_recognition_service) {
#if BUILDFLAG(IS_CHROMEOS)
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<SpeechRecognitionRecognizerImpl>(
          std::move(remote), std::move(options),
          binary_path, config_paths,
          primary_language_name, mask_offensive_words,
          speech_recognition_service),
      std::move(receiver));
#else
  auto receiver_ref = mojo::MakeSelfOwnedReceiver(
      std::make_unique<SpeechRecognitionRecognizerImpl>(
          std::move(remote), std::move(options),
          binary_path, config_paths,
          primary_language_name, mask_offensive_words,
          speech_recognition_service),
      std::move(receiver));
  auto* recognizer =
      static_cast<SpeechRecognitionRecognizerImpl*>(
          receiver_ref->impl());
  recognizer->CreateSodaClient(binary_path);
#endif
}

void SpeechRecognitionRecognizerImpl::CreateSodaClient(
    const base::FilePath& binary_path) {
  if (speech_recognition_service_) {
    // The actual instance is our subclass due to the
    // #define rename in the header.
    auto* service =
        static_cast<SpeechRecognitionServiceImpl*>(
            speech_recognition_service_.get());
    auto stream = service->TakeAsrStream();
    if (stream.is_valid()) {
      soda_client_ =
          std::make_unique<BraveSodaClient>(
              std::move(stream),
              service->TakeAsrResponder());
      return;
    }
  }

  // Fall back to real SODA.
  SpeechRecognitionRecognizerImpl_ChromiumImpl::
      CreateSodaClient(binary_path);
}

}  // namespace speech
