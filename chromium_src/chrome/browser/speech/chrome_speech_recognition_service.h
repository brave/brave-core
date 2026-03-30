/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_CHROME_SPEECH_RECOGNITION_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_CHROME_SPEECH_RECOGNITION_SERVICE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/on_device_speech_recognition.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

namespace speech {
class ChromeSpeechRecognitionService;
using ChromeSpeechRecognitionService_BraveImpl =
    ChromeSpeechRecognitionService;
}  // namespace speech

#define ChromeSpeechRecognitionService \
  ChromeSpeechRecognitionService_ChromiumImpl

#define LaunchIfNotRunning                         \
  LaunchIfNotRunning_Unused();                     \
  friend ChromeSpeechRecognitionService_BraveImpl; \
  void LaunchIfNotRunning

#include <chrome/browser/speech/chrome_speech_recognition_service.h>  // IWYU pragma: export

#undef LaunchIfNotRunning
#undef ChromeSpeechRecognitionService

namespace speech {

class ChromeSpeechRecognitionService
    : public ChromeSpeechRecognitionService_ChromiumImpl {
 public:
  explicit ChromeSpeechRecognitionService(
      content::BrowserContext* context);
  ~ChromeSpeechRecognitionService() override;

  ChromeSpeechRecognitionService(
      const ChromeSpeechRecognitionService&) = delete;
  ChromeSpeechRecognitionService& operator=(
      const ChromeSpeechRecognitionService&) = delete;

  // SpeechRecognitionService:
  void BindSpeechRecognitionContext(
      mojo::PendingReceiver<
          media::mojom::SpeechRecognitionContext>
          receiver) override;

 private:
  void OnAsrSessionStored(
      mojo::PendingReceiver<
          media::mojom::SpeechRecognitionContext>
          receiver);

  mojo::Remote<
      local_ai::mojom::OnDeviceSpeechRecognitionService>
      speech_service_;

  base::WeakPtrFactory<ChromeSpeechRecognitionService>
      weak_ptr_factory_{this};
};

}  // namespace speech

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_CHROME_SPEECH_RECOGNITION_SERVICE_H_
