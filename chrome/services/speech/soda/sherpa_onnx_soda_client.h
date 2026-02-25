/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROME_SERVICES_SPEECH_SODA_SHERPA_ONNX_SODA_CLIENT_H_
#define BRAVE_CHROME_SERVICES_SPEECH_SODA_SHERPA_ONNX_SODA_CLIENT_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_native_library.h"
#include "chrome/services/speech/soda/soda_async_impl.h"
#include "chrome/services/speech/soda/soda_client.h"

namespace soda {

// Drop-in replacement for SodaClientImpl that loads Sherpa-ONNX's C API
// shared library instead of Google's proprietary libsoda. Sherpa-ONNX is
// Apache 2.0 licensed and does not require an API key.
class SherpaOnnxSodaClient : public SodaClient {
 public:
  // Takes the fully-qualified path to the Sherpa-ONNX shared library
  // (e.g. libsherpa-onnx-c-api.dylib).
  explicit SherpaOnnxSodaClient(base::FilePath library_path);

  SherpaOnnxSodaClient(const SherpaOnnxSodaClient&) = delete;
  SherpaOnnxSodaClient& operator=(const SherpaOnnxSodaClient&) = delete;

  ~SherpaOnnxSodaClient() override;

  // SodaClient:
  void AddAudio(const char* audio_buffer, int audio_buffer_size) override;
  void MarkDone() override;
  bool DidAudioPropertyChange(int sample_rate, int channel_count) override;
  void Reset(const SerializedSodaConfig config,
             int sample_rate,
             int channel_count) override;
  void UpdateRecognitionContext(const RecognitionContext context) override;
  bool IsInitialized() override;
  bool BinaryLoadedSuccessfully() override;

  // Testing accessors — allow tests to exercise private helpers directly.
  void SetCallbackForTesting(SerializedSodaEventHandler callback,
                             void* callback_handle);
  void FireRecognitionResultForTesting(const std::string& text, bool is_final);
  void FireEndpointEventForTesting();
  void FireStopEventForTesting();
  std::vector<float> ConvertAndResampleAudioForTesting(
      const char* audio_buffer,
      int audio_buffer_size,
      int source_sample_rate,
      int channel_count);

 private:
  // Sherpa-ONNX C API function pointer typedefs.
  // All Sherpa-ONNX opaque types are represented as void* here since we load
  // the library dynamically and define the actual structs in the .cc file.
  using CreateOnlineRecognizerFn = const void* (*)(const void*);
  using DestroyOnlineRecognizerFn = void (*)(const void*);
  using CreateOnlineStreamFn = const void* (*)(const void*);
  using DestroyOnlineStreamFn = void (*)(const void*);
  using AcceptWaveformFn =
      void (*)(const void*, int32_t, const float*, int32_t);
  using DecodeOnlineStreamFn = void (*)(const void*, const void*);
  using GetOnlineStreamResultFn = const void* (*)(const void*, const void*);
  using DestroyOnlineRecognizerResultFn = void (*)(const void*);
  using OnlineStreamIsEndpointFn = int32_t (*)(const void*, const void*);
  using OnlineStreamResetFn = void (*)(const void*, const void*);
  using IsOnlineStreamReadyFn = int32_t (*)(const void*, const void*);

  // Fires a serialized SodaResponse via the stored callback.
  void FireRecognitionResult(const std::string& text, bool is_final);
  void FireEndpointEvent();
  void FireStopEvent();

  // Resamples audio from |source_rate| to 16000 Hz if needed.
  std::vector<float> ConvertAndResampleAudio(const char* audio_buffer,
                                             int audio_buffer_size,
                                             int source_sample_rate,
                                             int channel_count);

  base::ScopedNativeLibrary lib_;
  LoadSodaResultValue load_result_ = LoadSodaResultValue::kUnknown;

  // Sherpa-ONNX function pointers.
  CreateOnlineRecognizerFn create_recognizer_fn_ = nullptr;
  DestroyOnlineRecognizerFn destroy_recognizer_fn_ = nullptr;
  CreateOnlineStreamFn create_stream_fn_ = nullptr;
  DestroyOnlineStreamFn destroy_stream_fn_ = nullptr;
  AcceptWaveformFn accept_waveform_fn_ = nullptr;
  DecodeOnlineStreamFn decode_stream_fn_ = nullptr;
  GetOnlineStreamResultFn get_result_fn_ = nullptr;
  DestroyOnlineRecognizerResultFn destroy_result_fn_ = nullptr;
  OnlineStreamIsEndpointFn is_endpoint_fn_ = nullptr;
  OnlineStreamResetFn reset_stream_fn_ = nullptr;
  IsOnlineStreamReadyFn is_ready_fn_ = nullptr;

  // Sherpa-ONNX runtime state (opaque pointers, not owned by unique_ptr
  // because they must be freed via the library's own destroy functions).
  RAW_PTR_EXCLUSION const void* recognizer_ = nullptr;
  RAW_PTR_EXCLUSION const void* stream_ = nullptr;

  // SODA callback state (set during Reset).
  SerializedSodaEventHandler callback_ = nullptr;
  raw_ptr<void> callback_handle_ = nullptr;

  // Last recognized text, used to detect changes for partial results.
  std::string last_text_;

  // Cached model directory to avoid expensive recognizer recreation.
  std::string cached_model_dir_;

  bool is_initialized_ = false;
  int sample_rate_ = 0;
  int channel_count_ = 0;
};

}  // namespace soda

#endif  // BRAVE_CHROME_SERVICES_SPEECH_SODA_SHERPA_ONNX_SODA_CLIENT_H_
