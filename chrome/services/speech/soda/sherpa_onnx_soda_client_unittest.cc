/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/chrome/services/speech/soda/sherpa_onnx_soda_client.h"

#include <cstring>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "chrome/services/speech/soda/proto/soda_api.pb.h"
#include "chrome/services/speech/soda/soda_async_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace soda {
namespace {

class SherpaOnnxSodaClientTest : public testing::Test {
 protected:
  // Test that constructing with an invalid library path handles gracefully.
  void SetUp() override {}
};

TEST_F(SherpaOnnxSodaClientTest, InvalidLibraryPath) {
  SherpaOnnxSodaClient client(
      base::FilePath(FILE_PATH_LITERAL("/nonexistent/libsherpa-onnx.dylib")));
  EXPECT_FALSE(client.BinaryLoadedSuccessfully());
  EXPECT_FALSE(client.IsInitialized());
}

TEST_F(SherpaOnnxSodaClientTest, InvalidLibraryPathDoesNotCrashOnAddAudio) {
  SherpaOnnxSodaClient client(
      base::FilePath(FILE_PATH_LITERAL("/nonexistent/libsherpa-onnx.dylib")));
  // These should be no-ops when the library failed to load.
  const char audio[] = {0, 0, 0, 0};
  client.AddAudio(audio, sizeof(audio));
  client.MarkDone();
}

TEST_F(SherpaOnnxSodaClientTest, DidAudioPropertyChangeDetectsChanges) {
  SherpaOnnxSodaClient client(
      base::FilePath(FILE_PATH_LITERAL("/nonexistent/libsherpa-onnx.dylib")));
  // Default state: sample_rate=0, channel_count=0.
  EXPECT_TRUE(client.DidAudioPropertyChange(16000, 1));
  EXPECT_FALSE(client.DidAudioPropertyChange(0, 0));
}

TEST_F(SherpaOnnxSodaClientTest, ResetWithInvalidLibraryIsNoop) {
  SherpaOnnxSodaClient client(
      base::FilePath(FILE_PATH_LITERAL("/nonexistent/libsherpa-onnx.dylib")));

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

  // Should not crash.
  client.Reset(config, 16000, 1);
  EXPECT_FALSE(client.IsInitialized());
}

TEST_F(SherpaOnnxSodaClientTest,
       UpdateRecognitionContextWithInvalidLibraryIsNoop) {
  SherpaOnnxSodaClient client(
      base::FilePath(FILE_PATH_LITERAL("/nonexistent/libsherpa-onnx.dylib")));

  RecognitionContext context;
  context.recognition_context = nullptr;
  context.recognition_context_size = 0;

  // Should not crash.
  client.UpdateRecognitionContext(context);
}

}  // namespace
}  // namespace soda
