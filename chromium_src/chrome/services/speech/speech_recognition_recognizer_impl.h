/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_SERVICES_SPEECH_SPEECH_RECOGNITION_RECOGNIZER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_SERVICES_SPEECH_SPEECH_RECOGNITION_RECOGNIZER_IMPL_H_

// Forward-declare our subclass and create an alias so the
// friend injection below resolves to our class.
namespace speech {
class SpeechRecognitionRecognizerImpl;
using SpeechRecognitionRecognizerImpl_BraveImpl =
    SpeechRecognitionRecognizerImpl;
}  // namespace speech

// Rename the upstream class so all its methods go on
// _ChromiumImpl. Our subclass takes the original name.
#define SpeechRecognitionRecognizerImpl \
  SpeechRecognitionRecognizerImpl_ChromiumImpl

// Inject friend declaration and make CreateSodaClient
// virtual so the subclass override is used.
#define CreateSodaClient                                       \
  CreateSodaClient_Unused(const base::FilePath& binary_path);  \
  friend SpeechRecognitionRecognizerImpl_BraveImpl;            \
  virtual void CreateSodaClient

#include <chrome/services/speech/speech_recognition_recognizer_impl.h>  // IWYU pragma: export

#undef CreateSodaClient
#undef SpeechRecognitionRecognizerImpl

namespace speech {

// Subclass that intercepts Create() and CreateSodaClient()
// to use our engine when session pipes are available.
class SpeechRecognitionRecognizerImpl
    : public SpeechRecognitionRecognizerImpl_ChromiumImpl {
 public:
  using SpeechRecognitionRecognizerImpl_ChromiumImpl::
      SpeechRecognitionRecognizerImpl_ChromiumImpl;

  static void Create(
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
          speech_recognition_service);

  void CreateSodaClient(
      const base::FilePath& binary_path) override;
};

}  // namespace speech

#endif  // BRAVE_CHROMIUM_SRC_CHROME_SERVICES_SPEECH_SPEECH_RECOGNITION_RECOGNIZER_IMPL_H_
