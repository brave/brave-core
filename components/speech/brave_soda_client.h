/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEECH_BRAVE_SODA_CLIENT_H_
#define BRAVE_COMPONENTS_SPEECH_BRAVE_SODA_CLIENT_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/services/speech/soda/soda_client.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

namespace speech {

// SodaClient implementation that wraps Chromium's
// AsrStreamInput/AsrStreamResponder mojo interfaces.
// Forwards audio chunks to the speech WASM worker
// and receives transcripts asynchronously via
// AsrStreamResponder. Results are serialized as
// SodaResponse protos and delivered through the
// standard SODA callback mechanism.
class BraveSodaClient
    : public ::soda::SodaClient,
      public on_device_model::mojom::AsrStreamResponder {
 public:
  BraveSodaClient(
      mojo::PendingRemote<
          on_device_model::mojom::AsrStreamInput> stream,
      mojo::PendingReceiver<
          on_device_model::mojom::AsrStreamResponder>
          responder);
  ~BraveSodaClient() override;

  BraveSodaClient(const BraveSodaClient&) =
      delete;
  BraveSodaClient& operator=(
      const BraveSodaClient&) = delete;

  // soda::SodaClient:
  void AddAudio(const char* audio_buffer,
                int audio_buffer_size) override;
  void MarkDone() override;
  bool DidAudioPropertyChange(int sample_rate,
                              int channel_count) override;
  void Reset(const SerializedSodaConfig config,
             int sample_rate,
             int channel_count) override;
  void UpdateRecognitionContext(
      const RecognitionContext context) override;
  bool IsInitialized() override;
  bool BinaryLoadedSuccessfully() override;

  // on_device_model::mojom::AsrStreamResponder:
  void OnResponse(
      std::vector<
          on_device_model::mojom::SpeechRecognitionResultPtr>
          result) override;

 private:
  void SendStopEvent();

  mojo::Remote<on_device_model::mojom::AsrStreamInput>
      stream_;
  mojo::Receiver<on_device_model::mojom::AsrStreamResponder>
      responder_receiver_{this};
  SerializedSodaEventHandler callback_ = nullptr;
  raw_ptr<void> callback_handle_ = nullptr;
  int sample_rate_ = 0;
  int channel_count_ = 0;
  bool initialized_ = false;
};

}  // namespace speech

#endif  // BRAVE_COMPONENTS_SPEECH_BRAVE_SODA_CLIENT_H_
