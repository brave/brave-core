// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/content/browser/speech/brave_on_device_speech_recognition_engine.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "components/speech/audio_buffer.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_sample_types.h"
#include "media/base/channel_layout.h"
#include "media/base/converting_audio_fifo.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "services/on_device_model/public/mojom/on_device_model.mojom.h"

namespace content {

namespace {

// The speech worker only accepts 16 kHz mono audio.
constexpr int kModelSampleRateHz = 16000;

}  // namespace

BraveOnDeviceSpeechRecognitionEngine::BraveOnDeviceSpeechRecognitionEngine(
    const SpeechRecognitionSessionConfig& config)
    : OnDeviceSpeechRecognitionEngine(config) {
  // The embedder hands out sessions on the UI thread.
  GetUIThreadTaskRunner({})->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce([]() {
        return GetContentClient()->browser()->GetAsrSession();
      }),
      base::BindOnce(&BraveOnDeviceSpeechRecognitionEngine::OnAsrSessionReady,
                     brave_weak_factory_.GetWeakPtr()));
}

BraveOnDeviceSpeechRecognitionEngine::~BraveOnDeviceSpeechRecognitionEngine() =
    default;

void BraveOnDeviceSpeechRecognitionEngine::SetAudioParameters(
    media::AudioParameters audio_parameters) {
  // Audio from recognition.start(track) arrives at the track's own rate,
  // often 48 kHz, while microphone audio arrives already converted to 16 kHz
  // by SpeechRecognizerImpl. The worker only accepts 16 kHz, so convert track
  // audio here. ConvertingAudioFifo collects pieces of any size and converts
  // them with the same media::AudioConverter the microphone path uses.
  if (audio_parameters.sample_rate() != kModelSampleRateHz) {
    // Converted audio goes out in the same chunk size as microphone audio.
    // The chunk length is in milliseconds, so divide by 1000 to turn it into
    // a fraction of a second.
    const int frames_per_buffer =
        kModelSampleRateHz * GetDesiredAudioChunkDurationMs() / 1000;
    media::AudioParameters model_params(
        media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
        media::ChannelLayoutConfig::Mono(), kModelSampleRateHz,
        frames_per_buffer);
    resampler_fifo_ = std::make_unique<media::ConvertingAudioFifo>(
        audio_parameters, model_params);
    // Label the audio 16 kHz from here on, so the stream and every chunk tell
    // the worker the rate it really gets.
    audio_parameters = model_params;
  }

  // Call the grandparent, so the base class cannot pass the sample rate to its
  // Core and start an optimization guide session of its own.
  SpeechRecognitionEngine::SetAudioParameters(audio_parameters);
  // Starts the stream if the session remote has already arrived.
  TryCreateSession();
}

void BraveOnDeviceSpeechRecognitionEngine::TakeAudioChunk(
    const AudioChunk& data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);

  // No resampler means the audio is already 16 kHz, so the base takes it as
  // it is.
  if (!resampler_fifo_) {
    OnDeviceSpeechRecognitionEngine::TakeAudioChunk(data);
    return;
  }

  // Hand the audio to the resampler. It works on float samples, so convert
  // this chunk's int16 samples first.
  base::span<const int16_t> samples = data.SamplesData16AsSpan();
  auto bus =
      media::AudioBus::Create(/*channels=*/1, static_cast<int>(samples.size()));
  bus->FromInterleaved<media::SignedInt16SampleTypeTraits>(samples);
  resampler_fifo_->Push(std::move(bus));
  // The base gets converted audio in place of `data`, whenever the resampler
  // has a buffer ready. Most calls add too little to finish one.
  ForwardResampledAudio();
}

void BraveOnDeviceSpeechRecognitionEngine::ForwardResampledAudio() {
  // The base engine reads its chunks as int16 samples.
  std::vector<int16_t> resampled;
  while (resampler_fifo_->HasOutput()) {
    const media::AudioBus* out = resampler_fifo_->PeekOutput();
    const size_t offset = resampled.size();
    resampled.resize(offset + static_cast<size_t>(out->frames()));
    out->ToInterleaved<media::SignedInt16SampleTypeTraits>(
        base::span(resampled).subspan(offset));
    resampler_fifo_->PopOutput();
  }
  if (resampled.empty()) {
    return;
  }

  auto chunk = base::MakeRefCounted<AudioChunk>(base::as_byte_span(resampled),
                                                sizeof(int16_t));
  OnDeviceSpeechRecognitionEngine::TakeAudioChunk(*chunk);
}

void BraveOnDeviceSpeechRecognitionEngine::AudioChunksEnded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  // Send what the resampler still holds, before the stream closes below.
  if (resampler_fifo_) {
    resampler_fifo_->Flush();
    ForwardResampledAudio();
  }

  audio_ended_ = true;
  // Closing the input stream makes the worker emit its final result, so the
  // responder stays bound for it. Upstream would end recognition with an empty
  // result before that arrives, so we have to override this behavior to reply
  // with a final result from the worker instead.
  if (asr_stream_.is_bound()) {
    asr_stream_.reset();
    // Nothing else reports a worker that stays alive but never answers. The
    // timer is a member, so it cannot fire after `this` is destroyed.
    final_result_timer_.Start(
        FROM_HERE, kFinalResultTimeout,
        base::BindOnce(
            &BraveOnDeviceSpeechRecognitionEngine::OnFinalResultTimeout,
            base::Unretained(this)));
    return;
  }

  // No stream, so no final result is coming.
  OnDeviceSpeechRecognitionEngine::AudioChunksEnded();
}

void BraveOnDeviceSpeechRecognitionEngine::EndRecognition() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  final_result_timer_.Stop();
  OnDeviceSpeechRecognitionEngine::EndRecognition();
  // Drop any GetAsrSession reply still in flight, so it cannot start a stream.
  brave_weak_factory_.InvalidateWeakPtrs();
  asr_session_.reset();
}

void BraveOnDeviceSpeechRecognitionEngine::OnResponse(
    std::vector<on_device_model::mojom::SpeechRecognitionResultPtr> result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  // Any message means the worker is still alive and working, reset the timer.
  if (final_result_timer_.IsRunning()) {
    final_result_timer_.Reset();
  }

  // Nothing between here and blink consults interim_results, so the engine is
  // the only place that can honor it. Past the end of audio a provisional is
  // unwanted regardless: the recognizer reports it and then keeps waiting.
  if (audio_ended_ || !config_.interim_results) {
    const bool had_results = !result.empty();
    std::erase_if(result, [](const auto& r) { return !r->is_final; });
    // An empty vector means nothing was recognized, and that result is what
    // ends the session, so it still has to go through.
    if (had_results && result.empty()) {
      return;
    }
  }

  OnDeviceSpeechRecognitionEngine::OnResponse(std::move(result));
}

void BraveOnDeviceSpeechRecognitionEngine::OnFinalResultTimeout() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  // The empty result makes the recognizer end the session, which releases the
  // worker.
  OnDeviceSpeechRecognitionEngine::AudioChunksEnded();
}

void BraveOnDeviceSpeechRecognitionEngine::OnAsrSessionReady(
    mojo::PendingRemote<local_ai::mojom::AsrSession> pending) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  if (!pending.is_valid()) {
    return;
  }
  asr_session_.Bind(std::move(pending));
  // Starts the stream if the audio parameters have already arrived.
  TryCreateSession();
}

void BraveOnDeviceSpeechRecognitionEngine::TryCreateSession() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(main_sequence_checker_);
  if (session_created_ || !asr_session_.is_bound() ||
      !audio_parameters_.IsValid()) {
    return;
  }
  session_created_ = true;

  auto options = on_device_model::mojom::AsrStreamOptions::New();
  options->sample_rate_hz = audio_parameters_.sample_rate();
  if (!config_.language.empty()) {
    options->language = config_.language;
  }

  mojo::PendingRemote<on_device_model::mojom::AsrStreamInput> asr_stream;
  mojo::PendingReceiver<on_device_model::mojom::AsrStreamResponder>
      asr_stream_responder;
  asr_session_->Start(std::move(options),
                      asr_stream.InitWithNewPipeAndPassReceiver(),
                      asr_stream_responder.InitWithNewPipeAndPassRemote());

  // The base class owns both bindings and resets them in EndRecognition.
  OnAsrStreamCreated(std::move(asr_stream), std::move(asr_stream_responder));
}

}  // namespace content
