/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/speech/chrome_speech_recognition_service.h"

#include "base/feature_list.h"
#include "brave/browser/local_ai/on_device_speech_recognition_service_factory.h"
#include "brave/components/local_ai/core/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/service_process_host.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

#define ChromeSpeechRecognitionService \
  ChromeSpeechRecognitionService_ChromiumImpl

#include <chrome/browser/speech/chrome_speech_recognition_service.cc>

#undef ChromeSpeechRecognitionService

namespace speech {

namespace {
constexpr base::TimeDelta kBraveIdleProcessTimeout =
    base::Seconds(5);
}  // namespace

ChromeSpeechRecognitionService::
    ChromeSpeechRecognitionService(
        content::BrowserContext* context)
    : ChromeSpeechRecognitionService_ChromiumImpl(
          context) {}

ChromeSpeechRecognitionService::
    ~ChromeSpeechRecognitionService() = default;

void ChromeSpeechRecognitionService::
    BindSpeechRecognitionContext(
        mojo::PendingReceiver<
            media::mojom::SpeechRecognitionContext>
            receiver) {
  if (!base::FeatureList::IsEnabled(
          local_ai::features::
              kBraveOnDeviceSpeechRecognition)) {
    return ChromeSpeechRecognitionService_ChromiumImpl::
        BindSpeechRecognitionContext(
            std::move(receiver));
  }

  // Launch the speech recognition utility process if
  // not already running.
  if (!speech_recognition_service_.is_bound()) {
    content::ServiceProcessHost::Launch(
        speech_recognition_service_
            .BindNewPipeAndPassReceiver(),
        content::ServiceProcessHost::Options()
            .WithDisplayName(
                IDS_UTILITY_PROCESS_SPEECH_RECOGNITION_SERVICE_NAME)
            .Pass());
    speech_recognition_service_.reset_on_disconnect();
    speech_recognition_service_.reset_on_idle_timeout(
        kBraveIdleProcessTimeout);
  }

  // Get session from OnDeviceSpeechRecognitionService.
  auto* profile = Profile::FromBrowserContext(context());
  if (!speech_service_.is_bound()) {
    speech_service_.Bind(
        local_ai::
            OnDeviceSpeechRecognitionServiceFactory::
                GetForProfile(profile));
  }

  LOG(ERROR) << "[BraveSpeech] Creating ASR session";

  // Create pipe pairs.
  mojo::PendingRemote<
      on_device_model::mojom::AsrStreamInput>
      stream_remote;
  auto stream_receiver =
      stream_remote.InitWithNewPipeAndPassReceiver();

  mojo::PendingRemote<
      on_device_model::mojom::AsrStreamResponder>
      responder_remote;
  auto responder_receiver =
      responder_remote.InitWithNewPipeAndPassReceiver();

  // CreateSession — wait for WASM worker to be ready
  // and bind the pipes before sending the other ends
  // to the utility process.
  auto options =
      on_device_model::mojom::AsrStreamOptions::New();
  speech_service_->CreateSession(
      std::move(options), std::move(stream_receiver),
      std::move(responder_remote),
      base::BindOnce(
          &ChromeSpeechRecognitionService::
              OnSessionCreated,
          weak_ptr_factory_.GetWeakPtr(),
          std::move(receiver),
          std::move(stream_remote),
          std::move(responder_receiver)));
}

void ChromeSpeechRecognitionService::OnSessionCreated(
    mojo::PendingReceiver<
        media::mojom::SpeechRecognitionContext> receiver,
    mojo::PendingRemote<
        on_device_model::mojom::AsrStreamInput> stream,
    mojo::PendingReceiver<
        on_device_model::mojom::AsrStreamResponder>
        responder) {
  if (!speech_recognition_service_.is_bound()) {
    return;
  }

  LOG(ERROR) << "[BraveSpeech] Session created, "
             << "sending to utility process";
  speech_recognition_service_->SetAsrSession(
      std::move(stream), std::move(responder),
      base::BindOnce(
          &ChromeSpeechRecognitionService::
              OnAsrSessionStored,
          weak_ptr_factory_.GetWeakPtr(),
          std::move(receiver)));
}

void ChromeSpeechRecognitionService::
    OnAsrSessionStored(
        mojo::PendingReceiver<
            media::mojom::SpeechRecognitionContext>
            receiver) {
  if (!speech_recognition_service_.is_bound()) {
    return;
  }
  speech_recognition_service_
      ->BindSpeechRecognitionContext(
          std::move(receiver));
}

}  // namespace speech
