/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/chrome/services/speech/soda/sherpa_onnx_soda_client.h"

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "chrome/services/speech/soda/proto/soda_api.pb.h"
#include "chrome/services/speech/soda/soda_async_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace soda {
namespace {

// Helper to capture callback invocations from Fire* methods.
struct CallbackCapture {
  std::vector<std::string> responses;

  static void Handler(const char* response, int response_size, void* handle) {
    auto* self = static_cast<CallbackCapture*>(handle);
    self->responses.emplace_back(response, response_size);
  }
};

// Creates a client with an invalid library path (library won't load).
SherpaOnnxSodaClient CreateInvalidClient() {
  return SherpaOnnxSodaClient(
      base::FilePath(FILE_PATH_LITERAL("/nonexistent/libsherpa-onnx.so")));
}

// --- Graceful degradation tests (invalid library) ---

TEST(SherpaOnnxSodaClientTest, InvalidLibraryPath) {
  auto client = CreateInvalidClient();
  EXPECT_FALSE(client.BinaryLoadedSuccessfully());
  EXPECT_FALSE(client.IsInitialized());
}

TEST(SherpaOnnxSodaClientTest, InvalidLibraryPathDoesNotCrashOnAddAudio) {
  auto client = CreateInvalidClient();
  const char audio[] = {0, 0, 0, 0};
  client.AddAudio(audio, sizeof(audio));
  client.MarkDone();
}

TEST(SherpaOnnxSodaClientTest, DidAudioPropertyChangeDetectsChanges) {
  auto client = CreateInvalidClient();
  // Default state: sample_rate=0, channel_count=0.
  EXPECT_TRUE(client.DidAudioPropertyChange(16000, 1));
  EXPECT_FALSE(client.DidAudioPropertyChange(0, 0));
}

TEST(SherpaOnnxSodaClientTest, ResetWithInvalidLibraryIsNoop) {
  auto client = CreateInvalidClient();

  speech::soda::chrome::ExtendedSodaConfigMsg config_msg;
  config_msg.set_language_pack_directory("/tmp/fake_model");
  config_msg.set_sample_rate(16000);
  config_msg.set_channel_count(1);
  auto serialized = config_msg.SerializeAsString();

  SerializedSodaConfig config;
  config.soda_config = serialized.c_str();
  config.soda_config_size = static_cast<int>(serialized.size());
  config.callback = nullptr;
  config.callback_handle = nullptr;

  client.Reset(config, 16000, 1);
  EXPECT_FALSE(client.IsInitialized());
}

TEST(SherpaOnnxSodaClientTest,
     UpdateRecognitionContextWithInvalidLibraryIsNoop) {
  auto client = CreateInvalidClient();

  RecognitionContext context;
  context.recognition_context = nullptr;
  context.recognition_context_size = 0;

  client.UpdateRecognitionContext(context);
}

// --- Audio conversion tests ---

TEST(SherpaOnnxSodaClientTest, ConvertMonoAudio16kHz) {
  auto client = CreateInvalidClient();

  // 4 mono samples at 16kHz (int16_t PCM).
  const int16_t samples[] = {0, 16384, -16384, 32767};
  auto result = client.ConvertAndResampleAudioForTesting(
      reinterpret_cast<const char*>(samples), sizeof(samples), 16000, 1);

  ASSERT_EQ(result.size(), 4u);
  EXPECT_FLOAT_EQ(result[0], 0.0f);
  EXPECT_FLOAT_EQ(result[1], 16384.0f / 32768.0f);
  EXPECT_FLOAT_EQ(result[2], -16384.0f / 32768.0f);
  EXPECT_FLOAT_EQ(result[3], 32767.0f / 32768.0f);
}

TEST(SherpaOnnxSodaClientTest, ConvertStereoToMono) {
  auto client = CreateInvalidClient();

  // 2 stereo frames: (1000, 3000), (2000, 4000).
  // Expected mono: avg(1000,3000)/32768, avg(2000,4000)/32768.
  const int16_t samples[] = {1000, 3000, 2000, 4000};
  auto result = client.ConvertAndResampleAudioForTesting(
      reinterpret_cast<const char*>(samples), sizeof(samples), 16000, 2);

  ASSERT_EQ(result.size(), 2u);
  EXPECT_FLOAT_EQ(result[0], 2000.0f / 32768.0f);
  EXPECT_FLOAT_EQ(result[1], 3000.0f / 32768.0f);
}

TEST(SherpaOnnxSodaClientTest, ResampleFrom48kHzTo16kHz) {
  auto client = CreateInvalidClient();

  // Generate a simple 480-sample buffer at 48kHz (10ms of audio).
  // At 3:1 ratio, we expect ~160 output samples.
  std::vector<int16_t> samples(480);
  for (size_t i = 0; i < samples.size(); ++i) {
    // 1kHz sine wave at 48kHz sample rate.
    samples[i] = static_cast<int16_t>(
        16384.0 * std::sin(2.0 * M_PI * 1000.0 * i / 48000.0));
  }

  auto result = client.ConvertAndResampleAudioForTesting(
      reinterpret_cast<const char*>(samples.data()),
      static_cast<int>(samples.size() * sizeof(int16_t)), 48000, 1);

  // 48kHz -> 16kHz is 3:1 ratio, so 480/3 = 160 output frames.
  EXPECT_EQ(result.size(), 160u);
  // Verify output is not all zeros (resampling preserved the signal).
  float max_val = 0.0f;
  for (float sample : result) {
    max_val = std::max(max_val, std::abs(sample));
  }
  EXPECT_GT(max_val, 0.1f);
}

TEST(SherpaOnnxSodaClientTest, EmptyAudioBuffer) {
  auto client = CreateInvalidClient();

  auto result =
      client.ConvertAndResampleAudioForTesting(nullptr, 0, 16000, 1);
  EXPECT_TRUE(result.empty());
}

// --- Callback / event firing tests ---

TEST(SherpaOnnxSodaClientTest, FireRecognitionResultPartial) {
  auto client = CreateInvalidClient();
  CallbackCapture capture;
  client.SetCallbackForTesting(&CallbackCapture::Handler, &capture);

  client.FireRecognitionResultForTesting("hello world", /*is_final=*/false);

  ASSERT_EQ(capture.responses.size(), 1u);
  speech::soda::chrome::SodaResponse response;
  ASSERT_TRUE(response.ParseFromString(capture.responses[0]));
  EXPECT_EQ(response.soda_type(),
            speech::soda::chrome::SodaResponse::RECOGNITION);
  EXPECT_EQ(response.recognition_result().hypothesis(0), "hello world");
  EXPECT_EQ(response.recognition_result().result_type(),
            speech::soda::chrome::SodaRecognitionResult::PARTIAL);
}

TEST(SherpaOnnxSodaClientTest, FireRecognitionResultFinal) {
  auto client = CreateInvalidClient();
  CallbackCapture capture;
  client.SetCallbackForTesting(&CallbackCapture::Handler, &capture);

  client.FireRecognitionResultForTesting("final text", /*is_final=*/true);

  ASSERT_EQ(capture.responses.size(), 1u);
  speech::soda::chrome::SodaResponse response;
  ASSERT_TRUE(response.ParseFromString(capture.responses[0]));
  EXPECT_EQ(response.recognition_result().result_type(),
            speech::soda::chrome::SodaRecognitionResult::FINAL);
}

TEST(SherpaOnnxSodaClientTest, FireEndpointEvent) {
  auto client = CreateInvalidClient();
  CallbackCapture capture;
  client.SetCallbackForTesting(&CallbackCapture::Handler, &capture);

  client.FireEndpointEventForTesting();

  ASSERT_EQ(capture.responses.size(), 1u);
  speech::soda::chrome::SodaResponse response;
  ASSERT_TRUE(response.ParseFromString(capture.responses[0]));
  EXPECT_EQ(response.soda_type(),
            speech::soda::chrome::SodaResponse::ENDPOINT);
  EXPECT_EQ(response.endpoint_event().endpoint_type(),
            speech::soda::chrome::SodaEndpointEvent::END_OF_UTTERANCE);
}

TEST(SherpaOnnxSodaClientTest, FireStopEvent) {
  auto client = CreateInvalidClient();
  CallbackCapture capture;
  client.SetCallbackForTesting(&CallbackCapture::Handler, &capture);

  client.FireStopEventForTesting();

  ASSERT_EQ(capture.responses.size(), 1u);
  speech::soda::chrome::SodaResponse response;
  ASSERT_TRUE(response.ParseFromString(capture.responses[0]));
  EXPECT_EQ(response.soda_type(), speech::soda::chrome::SodaResponse::STOP);
}

TEST(SherpaOnnxSodaClientTest, FireWithNoCallbackIsNoop) {
  auto client = CreateInvalidClient();
  // No callback set — these should not crash.
  client.FireRecognitionResultForTesting("test", false);
  client.FireEndpointEventForTesting();
  client.FireStopEventForTesting();
}

}  // namespace
}  // namespace soda
