/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/chrome/services/speech/soda/sherpa_onnx_soda_client.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "partition_alloc/pointers/raw_ptr_exclusion.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "chrome/services/speech/soda/proto/soda_api.pb.h"
#include "media/base/sinc_resampler.h"

namespace soda {

namespace {

// Sherpa-ONNX C API struct definitions — must match the library's ABI.
// We define them here rather than including Sherpa-ONNX headers directly,
// since we load the library at runtime.

struct SherpaOnnxFeatureConfig {
  int32_t sample_rate;
  int32_t feature_dim;
};

struct SherpaOnnxOnlineTransducerModelConfig {
  const char* encoder;
  const char* decoder;
  const char* joiner;
};

struct SherpaOnnxOnlineParaformerModelConfig {
  const char* encoder;
  const char* decoder;
};

struct SherpaOnnxOnlineZipformer2CtcModelConfig {
  const char* model;
};

struct SherpaOnnxOnlineNemoCtcModelConfig {
  const char* model;
};

struct SherpaOnnxOnlineToneCtcModelConfig {
  const char* model;
};

struct SherpaOnnxOnlineModelConfig {
  SherpaOnnxOnlineTransducerModelConfig transducer;
  SherpaOnnxOnlineParaformerModelConfig paraformer;
  SherpaOnnxOnlineZipformer2CtcModelConfig zipformer2_ctc;
  const char* tokens;
  int32_t num_threads;
  const char* provider;
  int32_t debug;
  const char* model_type;
  const char* modeling_unit;
  const char* bpe_vocab;
  const char* tokens_buf;
  int32_t tokens_buf_size;
  SherpaOnnxOnlineNemoCtcModelConfig nemo_ctc;
  SherpaOnnxOnlineToneCtcModelConfig t_one_ctc;
};

struct SherpaOnnxOnlineCtcFstDecoderConfig {
  const char* graph;
  int32_t max_active;
};

struct SherpaOnnxHomophoneReplacerConfig {
  const char* rule_fst;
  const char* dict_dir;
};

struct SherpaOnnxOnlineRecognizerConfig {
  SherpaOnnxFeatureConfig feat_config;
  SherpaOnnxOnlineModelConfig model_config;
  const char* decoding_method;
  int32_t max_active_paths;
  int32_t enable_endpoint;
  float rule1_min_trailing_silence;
  float rule2_min_trailing_silence;
  float rule3_min_utterance_length;
  const char* hotwords_file;
  float hotwords_score;
  SherpaOnnxOnlineCtcFstDecoderConfig ctc_fst_decoder_config;
  const char* rule_fsts;
  const char* rule_fars;
  float blank_penalty;
  const char* hotwords_buf;
  int32_t hotwords_buf_size;
  SherpaOnnxHomophoneReplacerConfig hr;
};

struct SherpaOnnxOnlineRecognizerResult {
  const char* text;
  const char* tokens;
  RAW_PTR_EXCLUSION const char* const* tokens_arr;
  RAW_PTR_EXCLUSION float* timestamps;
  int32_t count;
  const char* json;
};

constexpr int kTargetSampleRate = 16000;

}  // namespace

SherpaOnnxSodaClient::SherpaOnnxSodaClient(base::FilePath library_path)
    : lib_(library_path) {
  if (!lib_.is_valid()) {
    LOG(ERROR) << "Sherpa-ONNX library at " << library_path.value()
               << " could not be loaded.";
    if (lib_.GetError()) {
      LOG(ERROR) << "Error: " << lib_.GetError()->ToString();
    }
    load_result_ = LoadSodaResultValue::kBinaryInvalid;
    base::UmaHistogramEnumeration("Accessibility.LiveCaption.LoadSodaResult",
                                  load_result_);
    return;
  }

  // Resolve all required function pointers.
  create_recognizer_fn_ = reinterpret_cast<CreateOnlineRecognizerFn>(
      lib_.GetFunctionPointer("SherpaOnnxCreateOnlineRecognizer"));
  destroy_recognizer_fn_ = reinterpret_cast<DestroyOnlineRecognizerFn>(
      lib_.GetFunctionPointer("SherpaOnnxDestroyOnlineRecognizer"));
  create_stream_fn_ = reinterpret_cast<CreateOnlineStreamFn>(
      lib_.GetFunctionPointer("SherpaOnnxCreateOnlineStream"));
  destroy_stream_fn_ = reinterpret_cast<DestroyOnlineStreamFn>(
      lib_.GetFunctionPointer("SherpaOnnxDestroyOnlineStream"));
  accept_waveform_fn_ = reinterpret_cast<AcceptWaveformFn>(
      lib_.GetFunctionPointer("SherpaOnnxOnlineStreamAcceptWaveform"));
  decode_stream_fn_ = reinterpret_cast<DecodeOnlineStreamFn>(
      lib_.GetFunctionPointer("SherpaOnnxDecodeOnlineStream"));
  get_result_fn_ = reinterpret_cast<GetOnlineStreamResultFn>(
      lib_.GetFunctionPointer("SherpaOnnxGetOnlineStreamResult"));
  destroy_result_fn_ = reinterpret_cast<DestroyOnlineRecognizerResultFn>(
      lib_.GetFunctionPointer("SherpaOnnxDestroyOnlineRecognizerResult"));
  is_endpoint_fn_ = reinterpret_cast<OnlineStreamIsEndpointFn>(
      lib_.GetFunctionPointer("SherpaOnnxOnlineStreamIsEndpoint"));
  reset_stream_fn_ = reinterpret_cast<OnlineStreamResetFn>(
      lib_.GetFunctionPointer("SherpaOnnxOnlineStreamReset"));
  is_ready_fn_ = reinterpret_cast<IsOnlineStreamReadyFn>(
      lib_.GetFunctionPointer("SherpaOnnxIsOnlineStreamReady"));

  if (!(create_recognizer_fn_ && destroy_recognizer_fn_ &&
        create_stream_fn_ && destroy_stream_fn_ && accept_waveform_fn_ &&
        decode_stream_fn_ && get_result_fn_ && destroy_result_fn_ &&
        is_endpoint_fn_ && reset_stream_fn_ && is_ready_fn_)) {
    LOG(ERROR) << "Failed to resolve one or more Sherpa-ONNX function pointers";
    load_result_ = LoadSodaResultValue::kFunctionPointerInvalid;
    base::UmaHistogramEnumeration("Accessibility.LiveCaption.LoadSodaResult",
                                  load_result_);
    return;
  }

  load_result_ = LoadSodaResultValue::kSuccess;
  base::UmaHistogramEnumeration("Accessibility.LiveCaption.LoadSodaResult",
                                load_result_);
}

NO_SANITIZE("cfi-icall")
SherpaOnnxSodaClient::~SherpaOnnxSodaClient() {
  if (stream_ && destroy_stream_fn_) {
    destroy_stream_fn_(stream_);
  }
  if (recognizer_ && destroy_recognizer_fn_) {
    destroy_recognizer_fn_(recognizer_);
  }
}

NO_SANITIZE("cfi-icall")
void SherpaOnnxSodaClient::AddAudio(const char* audio_buffer,
                                     int audio_buffer_size) {
  if (load_result_ != LoadSodaResultValue::kSuccess || !stream_) {
    return;
  }

  std::vector<float> samples = ConvertAndResampleAudio(
      audio_buffer, audio_buffer_size, sample_rate_, channel_count_);

  if (samples.empty()) {
    return;
  }

  accept_waveform_fn_(stream_, kTargetSampleRate, samples.data(),
                       static_cast<int32_t>(samples.size()));

  // Must check IsOnlineStreamReady before calling decode, otherwise
  // Sherpa-ONNX will crash with an out-of-bounds assertion.
  while (is_ready_fn_(recognizer_, stream_)) {
    decode_stream_fn_(recognizer_, stream_);
  }

  // Poll for results.
  const void* raw_result = get_result_fn_(recognizer_, stream_);
  if (raw_result) {
    const auto* result =
        static_cast<const SherpaOnnxOnlineRecognizerResult*>(raw_result);
    if (result->text && result->text[0] != '\0') {
      std::string text(result->text);
      if (text != last_text_) {
        last_text_ = text;
        FireRecognitionResult(text, /*is_final=*/false);
      }
    }
    destroy_result_fn_(raw_result);
  }

  // Check for endpoint (end of utterance).
  if (is_endpoint_fn_(recognizer_, stream_)) {
    if (!last_text_.empty()) {
      FireRecognitionResult(last_text_, /*is_final=*/true);
      last_text_.clear();
    }
    FireEndpointEvent();
    reset_stream_fn_(recognizer_, stream_);
  }
}

NO_SANITIZE("cfi-icall")
void SherpaOnnxSodaClient::MarkDone() {
  if (load_result_ != LoadSodaResultValue::kSuccess) {
    return;
  }

  if (!last_text_.empty()) {
    FireRecognitionResult(last_text_, /*is_final=*/true);
    last_text_.clear();
  }
  FireStopEvent();
}

bool SherpaOnnxSodaClient::DidAudioPropertyChange(int sample_rate,
                                                    int channel_count) {
  return sample_rate != sample_rate_ || channel_count != channel_count_;
}

NO_SANITIZE("cfi-icall")
void SherpaOnnxSodaClient::Reset(const SerializedSodaConfig config,
                                  int sample_rate,
                                  int channel_count) {
  if (load_result_ != LoadSodaResultValue::kSuccess) {
    return;
  }

  // Clean up previous stream but keep recognizer if model hasn't changed.
  if (stream_ && destroy_stream_fn_) {
    destroy_stream_fn_(stream_);
    stream_ = nullptr;
  }
  last_text_.clear();

  // Store callback for firing SodaResponse events.
  callback_ = config.callback;
  callback_handle_ = config.callback_handle;
  sample_rate_ = sample_rate;
  channel_count_ = channel_count;

  // Parse the ExtendedSodaConfigMsg to extract the model directory.
  speech::soda::chrome::ExtendedSodaConfigMsg config_msg;
  if (!config_msg.ParseFromArray(config.soda_config,
                                 config.soda_config_size)) {
    LOG(WARNING) << "Failed to parse ExtendedSodaConfigMsg";
    return;
  }

  std::string model_dir = config_msg.language_pack_directory();
  if (model_dir.empty()) {
    LOG(WARNING) << "No language_pack_directory in config";
    return;
  }

  // Reuse existing recognizer if model directory hasn't changed.
  if (recognizer_ && model_dir == cached_model_dir_) {
    // Fast path: recognizer already loaded.
  } else {
    // Model directory changed or first init — recreate recognizer.
    if (recognizer_ && destroy_recognizer_fn_) {
      destroy_recognizer_fn_(recognizer_);
      recognizer_ = nullptr;
    }

    // Build model file paths.
    base::FilePath model_path(model_dir);
    std::string encoder_path =
        model_path.Append(FILE_PATH_LITERAL("encoder.onnx")).AsUTF8Unsafe();
    std::string decoder_path =
        model_path.Append(FILE_PATH_LITERAL("decoder.onnx")).AsUTF8Unsafe();
    std::string joiner_path =
        model_path.Append(FILE_PATH_LITERAL("joiner.onnx")).AsUTF8Unsafe();
    std::string tokens_path =
        model_path.Append(FILE_PATH_LITERAL("tokens.txt")).AsUTF8Unsafe();

    // Configure Sherpa-ONNX recognizer.
    SherpaOnnxOnlineRecognizerConfig sherpa_config = {};

    sherpa_config.feat_config.sample_rate = kTargetSampleRate;
    sherpa_config.feat_config.feature_dim = 80;

    sherpa_config.model_config.transducer.encoder = encoder_path.c_str();
    sherpa_config.model_config.transducer.decoder = decoder_path.c_str();
    sherpa_config.model_config.transducer.joiner = joiner_path.c_str();
    sherpa_config.model_config.paraformer.encoder = "";
    sherpa_config.model_config.paraformer.decoder = "";
    sherpa_config.model_config.zipformer2_ctc.model = "";
    sherpa_config.model_config.nemo_ctc.model = "";
    sherpa_config.model_config.t_one_ctc.model = "";
    sherpa_config.model_config.tokens = tokens_path.c_str();
    sherpa_config.model_config.num_threads = 2;
    sherpa_config.model_config.provider = "cpu";
    sherpa_config.model_config.debug = 0;
    sherpa_config.model_config.model_type = "";
    sherpa_config.model_config.modeling_unit = "";
    sherpa_config.model_config.bpe_vocab = "";
    sherpa_config.model_config.tokens_buf = "";
    sherpa_config.model_config.tokens_buf_size = 0;

    sherpa_config.decoding_method = "modified_beam_search";
    sherpa_config.max_active_paths = 4;
    sherpa_config.enable_endpoint = 1;
    sherpa_config.rule1_min_trailing_silence = 2.4f;
    sherpa_config.rule2_min_trailing_silence = 1.2f;
    sherpa_config.rule3_min_utterance_length = 20.0f;
    sherpa_config.hotwords_file = "";
    sherpa_config.hotwords_score = 0.0f;
    sherpa_config.ctc_fst_decoder_config.graph = "";
    sherpa_config.ctc_fst_decoder_config.max_active = 0;
    sherpa_config.rule_fsts = "";
    sherpa_config.rule_fars = "";
    sherpa_config.blank_penalty = 2.0f;
    sherpa_config.hotwords_buf = "";
    sherpa_config.hotwords_buf_size = 0;
    sherpa_config.hr.rule_fst = "";
    sherpa_config.hr.dict_dir = "";

    recognizer_ = create_recognizer_fn_(&sherpa_config);
    if (!recognizer_) {
      LOG(ERROR) << "Failed to create Sherpa-ONNX online recognizer. "
                 << "Model dir: " << model_dir;
      return;
    }
    cached_model_dir_ = model_dir;
  }

  stream_ = create_stream_fn_(recognizer_);
  if (!stream_) {
    LOG(ERROR) << "Failed to create Sherpa-ONNX online stream";
    destroy_recognizer_fn_(recognizer_);
    recognizer_ = nullptr;
    return;
  }

  is_initialized_ = true;
}

void SherpaOnnxSodaClient::UpdateRecognitionContext(
    const RecognitionContext context) {
  // Sherpa-ONNX does not support dynamic recognition context updates.
  // This is a no-op, matching optional behavior in SodaClientImpl.
}

bool SherpaOnnxSodaClient::IsInitialized() {
  return is_initialized_;
}

bool SherpaOnnxSodaClient::BinaryLoadedSuccessfully() {
  return load_result_ == LoadSodaResultValue::kSuccess;
}

void SherpaOnnxSodaClient::FireRecognitionResult(const std::string& text,
                                                   bool is_final) {
  if (!callback_) {
    return;
  }

  speech::soda::chrome::SodaResponse response;
  response.set_soda_type(speech::soda::chrome::SodaResponse::RECOGNITION);
  auto* result = response.mutable_recognition_result();
  result->add_hypothesis(text);
  result->set_result_type(
      is_final ? speech::soda::chrome::SodaRecognitionResult::FINAL
               : speech::soda::chrome::SodaRecognitionResult::PARTIAL);

  std::string serialized = response.SerializeAsString();
  callback_(serialized.c_str(), static_cast<int>(serialized.size()),
            callback_handle_);
}

void SherpaOnnxSodaClient::FireEndpointEvent() {
  if (!callback_) {
    return;
  }

  speech::soda::chrome::SodaResponse response;
  response.set_soda_type(speech::soda::chrome::SodaResponse::ENDPOINT);
  auto* endpoint = response.mutable_endpoint_event();
  endpoint->set_endpoint_type(
      speech::soda::chrome::SodaEndpointEvent::END_OF_UTTERANCE);

  std::string serialized = response.SerializeAsString();
  callback_(serialized.c_str(), static_cast<int>(serialized.size()),
            callback_handle_);
}

void SherpaOnnxSodaClient::FireStopEvent() {
  if (!callback_) {
    return;
  }

  speech::soda::chrome::SodaResponse response;
  response.set_soda_type(speech::soda::chrome::SodaResponse::STOP);

  std::string serialized = response.SerializeAsString();
  callback_(serialized.c_str(), static_cast<int>(serialized.size()),
            callback_handle_);
}

std::vector<float> SherpaOnnxSodaClient::ConvertAndResampleAudio(
    const char* audio_buffer,
    int audio_buffer_size,
    int source_sample_rate,
    int channel_count) {
  // Input is int16_t PCM.
  // SAFETY: audio_buffer contains audio_buffer_size bytes of int16_t PCM data.
  auto samples = UNSAFE_BUFFERS(
      base::span(reinterpret_cast<const int16_t*>(audio_buffer),
                 audio_buffer_size / sizeof(int16_t)));

  if (samples.empty()) {
    return {};
  }

  size_t frames = samples.size() / static_cast<size_t>(std::max(channel_count, 1));

  // Mix to mono if multi-channel.
  std::vector<float> mono(frames);
  if (channel_count <= 1) {
    for (size_t i = 0; i < frames; ++i) {
      mono[i] = static_cast<float>(samples[i]) / 32768.0f;
    }
  } else {
    for (size_t i = 0; i < frames; ++i) {
      float sum = 0.0f;
      for (int ch = 0; ch < channel_count; ++ch) {
        sum += static_cast<float>(samples[i * channel_count + ch]);
      }
      mono[i] = (sum / channel_count) / 32768.0f;
    }
  }

  // Resample to 16kHz if needed.
  if (source_sample_rate == kTargetSampleRate) {
    return mono;
  }

  double io_ratio =
      static_cast<double>(source_sample_rate) / kTargetSampleRate;
  int output_frames =
      static_cast<int>(std::ceil(static_cast<double>(frames) / io_ratio));

  if (output_frames <= 0) {
    return {};
  }

  // Use SincResampler for high-quality resampling.
  std::vector<float> resampled(output_frames);
  size_t read_pos = 0;
  const std::vector<float>& source = mono;

  media::SincResampler resampler(
      io_ratio, media::SincResampler::kDefaultRequestSize,
      base::BindRepeating(
          [](const std::vector<float>* src, size_t* pos, size_t src_frames,
             int frames_to_fill, float* destination) {
            // SAFETY: SincResampler guarantees destination has
            // frames_to_fill floats.
            auto dest = UNSAFE_BUFFERS(base::span(
                destination, static_cast<size_t>(frames_to_fill)));
            size_t available =
                std::min(static_cast<size_t>(frames_to_fill),
                         src_frames - *pos);
            auto src_span = base::span(*src).subspan(*pos, available);
            dest.first(available).copy_from(src_span);
            std::ranges::fill(dest.subspan(available), 0.0f);
            *pos += available;
          },
          &source, &read_pos, frames));

  resampler.Resample(base::span<float>(resampled));
  return resampled;
}

void SherpaOnnxSodaClient::SetCallbackForTesting(
    SerializedSodaEventHandler callback,
    void* callback_handle) {
  callback_ = callback;
  callback_handle_ = callback_handle;
}

void SherpaOnnxSodaClient::FireRecognitionResultForTesting(
    const std::string& text,
    bool is_final) {
  FireRecognitionResult(text, is_final);
}

void SherpaOnnxSodaClient::FireEndpointEventForTesting() {
  FireEndpointEvent();
}

void SherpaOnnxSodaClient::FireStopEventForTesting() {
  FireStopEvent();
}

std::vector<float> SherpaOnnxSodaClient::ConvertAndResampleAudioForTesting(
    const char* audio_buffer,
    int audio_buffer_size,
    int source_sample_rate,
    int channel_count) {
  return ConvertAndResampleAudio(audio_buffer, audio_buffer_size,
                                 source_sample_rate, channel_count);
}

}  // namespace soda
