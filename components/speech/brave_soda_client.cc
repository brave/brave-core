/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/speech/brave_soda_client.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "chrome/services/speech/soda/proto/soda_api.pb.h"

namespace speech {

BraveSodaClient::BraveSodaClient(
    mojo::PendingRemote<
        on_device_model::mojom::AsrStreamInput> stream,
    mojo::PendingReceiver<
        on_device_model::mojom::AsrStreamResponder>
        responder)
    : stream_(std::move(stream)),
      responder_receiver_(this, std::move(responder)) {
  responder_receiver_.set_disconnect_handler(
      base::BindOnce(&BraveSodaClient::OnResponderDisconnect,
                     base::Unretained(this)));
}

BraveSodaClient::~BraveSodaClient() =
    default;

void BraveSodaClient::AddAudio(
    const char* audio_buffer,
    int audio_buffer_size) {
  if (!initialized_ || !stream_.is_bound()) {
    LOG(ERROR) << "BraveSodaClient::AddAudio skipped"
               << " initialized=" << initialized_
               << " bound=" << stream_.is_bound();
    return;
  }
  LOG(ERROR) << "BraveSodaClient::AddAudio"
             << " bytes=" << audio_buffer_size;

  // Recover the original int16_t samples that the caller
  // cast to char* for the SODA C API.
  // SAFETY: audio_buffer points to audio_buffer_size
  // bytes of int16_t PCM data from the audio pipeline,
  // validated by CheckMul at the call site.
  auto samples = UNSAFE_BUFFERS(base::span(
      reinterpret_cast<const int16_t*>(audio_buffer),
      audio_buffer_size / sizeof(int16_t)));

  // Convert int16 PCM to float32 for AsrStreamInput.
  constexpr float kInt16ToFloatNormalizer = 32768.0f;
  auto audio_data =
      on_device_model::mojom::AudioData::New();
  audio_data->channel_count = channel_count_;
  audio_data->sample_rate = sample_rate_;
  audio_data->frame_count =
      static_cast<uint32_t>(samples.size());
  audio_data->data.reserve(samples.size());
  for (int16_t s : samples) {
    audio_data->data.push_back(
        s / kInt16ToFloatNormalizer);
  }

  stream_->AddAudioChunk(std::move(audio_data));
}

void BraveSodaClient::MarkDone() {
  LOG(ERROR) << "BraveSodaClient::MarkDone";
  if (!initialized_) {
    return;
  }
  // Disconnect the stream to signal end of audio.
  // The worker will flush remaining audio, send final
  // result via OnResponse (is_final=true), then
  // disconnect the responder pipe.
  stream_.reset();
  mark_done_pending_ = true;
}

bool BraveSodaClient::DidAudioPropertyChange(
    int sample_rate,
    int channel_count) {
  return sample_rate != sample_rate_ ||
         channel_count != channel_count_;
}

void BraveSodaClient::Reset(
    const SerializedSodaConfig config,
    int sample_rate,
    int channel_count) {
  LOG(ERROR) << "BraveSodaClient::Reset"
             << " sample_rate=" << sample_rate
             << " channel_count=" << channel_count;
  callback_ = config.callback;
  callback_handle_ = config.callback_handle;
  sample_rate_ = sample_rate;
  channel_count_ = channel_count;
  initialized_ = true;
}

void BraveSodaClient::UpdateRecognitionContext(
    const RecognitionContext context) {
  // Whisper does not support recognition context biasing.
}

bool BraveSodaClient::IsInitialized() {
  return initialized_;
}

bool BraveSodaClient::BinaryLoadedSuccessfully() {
  return true;
}

void BraveSodaClient::OnResponse(
    std::vector<
        on_device_model::mojom::SpeechRecognitionResultPtr>
        result) {
  if (!callback_) {
    return;
  }

  for (const auto& r : result) {
    if (r->transcript.empty()) {
      continue;
    }

    LOG(ERROR) << "BraveSodaClient::OnResponse"
               << " transcript='" << r->transcript << "'"
               << " is_final=" << r->is_final;

    soda::chrome::SodaResponse response;
    response.set_soda_type(
        soda::chrome::SodaResponse::RECOGNITION);
    auto* recognition_result =
        response.mutable_recognition_result();
    recognition_result->add_hypothesis(r->transcript);
    recognition_result->set_result_type(
        r->is_final
            ? soda::chrome::SodaRecognitionResult::FINAL
            : soda::chrome::SodaRecognitionResult::
                  PARTIAL);

    std::string serialized = response.SerializeAsString();
    callback_(serialized.data(),
              static_cast<int>(serialized.size()),
              callback_handle_.get());

    if (r->is_final && mark_done_pending_) {
      mark_done_pending_ = false;
      SendStopEvent();
    }
  }
}

void BraveSodaClient::OnResponderDisconnect() {
  LOG(ERROR) << "BraveSodaClient::OnResponderDisconnect";
  if (mark_done_pending_) {
    mark_done_pending_ = false;
    SendStopEvent();
  }
}

void BraveSodaClient::SendStopEvent() {
  if (!callback_) {
    return;
  }

  soda::chrome::SodaResponse response;
  response.set_soda_type(soda::chrome::SodaResponse::STOP);

  std::string serialized = response.SerializeAsString();
  callback_(serialized.data(),
            static_cast<int>(serialized.size()),
            callback_handle_.get());
}

}  // namespace speech
