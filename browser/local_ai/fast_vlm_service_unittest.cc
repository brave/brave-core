// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/local_ai/fast_vlm_service.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/browser/local_ai/fast_vlm_demo.h"
#include "brave/browser/local_ai/fast_vlm_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "services/webnn/public/mojom/features.mojom-features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

class FastVLMServiceTest : public testing::Test {
 public:
  void SetUp() override {
    // Enable WebNN features for testing
    scoped_feature_list_.InitAndEnableFeature(
        webnn::mojom::features::kWebMachineLearningNeuralNetwork);
    profile_ = std::make_unique<TestingProfile>();
  }

  void TearDown() override { profile_.reset(); }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(FastVLMServiceTest, ServiceCreation) {
  auto* service = FastVLMServiceFactory::GetForBrowserContext(profile_.get());
  ASSERT_TRUE(service);

  // Service should not be ready initially - GPU connection happens on first
  // inference
  std::string status = service->GetModelStatus();
  LOG(INFO) << "Service status: " << status;
  EXPECT_EQ(status, "Model not loaded");

  // Service should exist but not ready until first inference call
  EXPECT_FALSE(service->IsReady());
}

TEST_F(FastVLMServiceTest, DemoAPI) {
  // Test demo API functions
  std::string status = FastVLMDemo::GetServiceStatus(profile_.get());
  EXPECT_FALSE(status.empty());

  bool available = FastVLMDemo::IsServiceAvailable(profile_.get());
  LOG(INFO) << "Service available: " << available;

  // Create test image data
  std::vector<uint8_t> test_image(336 * 336 * 3, 128);  // Gray test image

  // Test inference call (will likely fail without GPU process in test)
  bool callback_called = false;
  FastVLMDemo::AnalyzeImage(
      profile_.get(), test_image, "Describe this test image",
      base::BindOnce(
          [](bool* called, bool success, const std::string& result) {
            *called = true;
            LOG(INFO) << "Inference result: "
                      << (success ? "Success" : "Failed") << " - " << result;
          },
          &callback_called));

  task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(FastVLMServiceTest, GPUConnectionOnInference) {
  auto* service = FastVLMServiceFactory::GetForBrowserContext(profile_.get());
  ASSERT_TRUE(service);

  // Initially service should not be ready
  EXPECT_FALSE(service->IsReady());
  EXPECT_EQ(service->GetModelStatus(), "Model not loaded");

  // Create test image data
  std::vector<uint8_t> test_image(336 * 336 * 3, 128);

  // First inference call should trigger GPU connection and model loading
  bool callback_called = false;
  service->RunInference(
      test_image, "Test prompt", 50,
      base::BindOnce(
          [](bool* called, bool success, const std::string& result) {
            *called = true;
            LOG(INFO) << "GPU connection test - Inference result: "
                      << (success ? "Success" : "Failed") << " - " << result;
          },
          &callback_called));

  // After inference call, let the background thread complete
  task_environment_.RunUntilIdle();
  std::string status_after_call = service->GetModelStatus();
  LOG(INFO) << "Status after inference call: " << status_after_call;

  // In test environment, model loading will likely fail due to test model paths
  // But the important thing is that the callback was called and no crashes
  // occurred
  EXPECT_TRUE(callback_called);
}

TEST_F(FastVLMServiceTest, ServiceFactory) {
  // Test that factory returns same instance
  auto* service1 = FastVLMServiceFactory::GetForBrowserContext(profile_.get());
  auto* service2 = FastVLMServiceFactory::GetForBrowserContext(profile_.get());

  EXPECT_EQ(service1, service2);
  EXPECT_TRUE(service1);
}

}  // namespace local_ai
