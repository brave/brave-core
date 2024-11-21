// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/model_validator.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/numerics/checked_math.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {
struct EndpointTestParams {
  std::string url;
  bool expected_with_private_ips_enabled;
  bool expected_with_private_ips_disabled;
};
}  // namespace
// Test fixture class for ModelValidator
class ModelValidatorUnitTest : public ::testing::Test {};

// Test IsValidContextSize with various context sizes
TEST_F(ModelValidatorUnitTest, IsValidContextSize) {
  struct TestCase {
    std::optional<int32_t> context_size;
    bool expected_result;
  };

  static constexpr auto kMaxCustomContextSize = kMaxCustomModelContextSize;

  base::CheckedNumeric<int32_t> kMaxCustomContextSizePlusOne =
      base::CheckAdd(kMaxCustomContextSize, 1);  // Check for overflow

  TestCase test_cases[] = {
      {1000, true},
      {kMinCustomModelContextSize, true},  // Minimum valid size
      {std::nullopt, false},               // Nullopt should return false
      {0, false},  // Invalid size (less than MIN_CONTEXT_SIZE)
      {kMaxCustomContextSize, true},                       // Max valid size
      {kMaxCustomContextSizePlusOne.ValueOrDie(), false},  // Beyond max size
  };

  for (const auto& test_case : test_cases) {
    bool actual = ModelValidator::IsValidContextSize(test_case.context_size);
    EXPECT_EQ(actual, test_case.expected_result)
        << "Failed for context_size = "
        << (test_case.context_size.has_value()
                ? base::NumberToString(*test_case.context_size)
                : "nullopt");
  }
}

// Test HasValidContextSize with various models
TEST_F(ModelValidatorUnitTest, HasValidContextSize) {
  // Valid custom model
  mojom::CustomModelOptionsPtr valid_custom_options =
      mojom::CustomModelOptions::New();
  valid_custom_options->context_size = 5000;

  mojom::Model valid_custom_model;
  valid_custom_model.options = mojom::ModelOptions::NewCustomModelOptions(
      std::move(valid_custom_options));

  EXPECT_EQ(ModelValidator::HasValidContextSize(
                *valid_custom_model.options->get_custom_model_options()),
            true);

  // Invalid context size
  mojom::CustomModelOptionsPtr invalid_context_size_options =
      mojom::CustomModelOptions::New();
  invalid_context_size_options->context_size = 0;  // Invalid context size

  mojom::Model invalid_context_size_model;
  invalid_context_size_model.options =
      mojom::ModelOptions::NewCustomModelOptions(
          std::move(invalid_context_size_options));

  EXPECT_EQ(
      ModelValidator::HasValidContextSize(
          *invalid_context_size_model.options->get_custom_model_options()),
      false);
}

// Test ValidateModel with valid and invalid models
TEST_F(ModelValidatorUnitTest, ValidateModel) {
  // Valid custom model
  mojom::CustomModelOptionsPtr valid_custom_options =
      mojom::CustomModelOptions::New();
  valid_custom_options->context_size = 5000;
  valid_custom_options->endpoint = GURL("https://valid-url.com");

  mojom::Model valid_custom_model;
  valid_custom_model.options = mojom::ModelOptions::NewCustomModelOptions(
      std::move(valid_custom_options));

  EXPECT_EQ(ModelValidator::ValidateCustomModelOptions(
                *valid_custom_model.options->get_custom_model_options()),
            ModelValidationResult::kSuccess);

  // Invalid context size
  mojom::CustomModelOptionsPtr invalid_context_size_options =
      mojom::CustomModelOptions::New();
  invalid_context_size_options->context_size = 0;  // Invalid context size
  invalid_context_size_options->endpoint = GURL("https://valid-url.com");

  mojom::Model invalid_context_size_model;
  invalid_context_size_model.options =
      mojom::ModelOptions::NewCustomModelOptions(
          std::move(invalid_context_size_options));

  EXPECT_EQ(
      ModelValidator::ValidateCustomModelOptions(
          *invalid_context_size_model.options->get_custom_model_options()),
      ModelValidationResult::kInvalidContextSize);

  // Invalid endpoint
  mojom::CustomModelOptionsPtr invalid_endpoint_options =
      mojom::CustomModelOptions::New();
  invalid_endpoint_options->context_size = 5000;
  invalid_endpoint_options->endpoint = GURL(
      "http://invalid-url.com");  // Invalid endpoint (not HTTPS or localhost)

  mojom::Model invalid_endpoint_model;
  invalid_endpoint_model.options = mojom::ModelOptions::NewCustomModelOptions(
      std::move(invalid_endpoint_options));

  EXPECT_EQ(ModelValidator::ValidateCustomModelOptions(
                *invalid_endpoint_model.options->get_custom_model_options()),
            ModelValidationResult::kInvalidUrl);
}

class ModelValidatorEndpointTest
    : public ::testing::TestWithParam<EndpointTestParams> {};

TEST_P(ModelValidatorEndpointTest, IsValidEndpoint) {
  const EndpointTestParams& params = GetParam();

  // Test with private IPs enabled
  {
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndEnableFeature(
        ai_chat::features::kAllowPrivateIPs);

    GURL endpoint(params.url);
    bool actual = ModelValidator::IsValidEndpoint(endpoint);
    EXPECT_EQ(actual, params.expected_with_private_ips_enabled)
        << "Failed for URL: " << params.url;
  }

  // Test with private IPs disabled
  {
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndDisableFeature(
        ai_chat::features::kAllowPrivateIPs);

    GURL endpoint(params.url);
    bool actual = ModelValidator::IsValidEndpoint(endpoint);
    EXPECT_EQ(actual, params.expected_with_private_ips_disabled)
        << "Failed for URL: " << params.url;
  }
}

INSTANTIATE_TEST_SUITE_P(
    ModelValidatorEndpointTests,
    ModelValidatorEndpointTest,
    ::testing::Values(
        // Legacy scenarios
        EndpointTestParams{"https://valid-url.com", true, true},
        EndpointTestParams{"https://localhost:8080", true, true},
        EndpointTestParams{"https://", false, false},
        EndpointTestParams{"https://search.brave.com/search?q=foo", true, true},
        // Localhost
        EndpointTestParams{"http://localhost", true, true},
        EndpointTestParams{"https://localhost", true, true},
        EndpointTestParams{"http://127.0.0.1", true, true},
        EndpointTestParams{"https://127.0.0.1", true, true},
        EndpointTestParams{"http://[::1]", true, true},
        EndpointTestParams{"https://[::1]", true, true},
        // Private IPv4 Addresses
        EndpointTestParams{"http://192.168.0.1", true, false},
        EndpointTestParams{"http://172.16.0.1", true, false},
        EndpointTestParams{"http://10.0.0.1", true, false},
        EndpointTestParams{"http://169.254.0.1", true, false},
        EndpointTestParams{"https://192.168.0.1", true, true},
        EndpointTestParams{"https://172.16.0.1", true, true},
        // Private IPv6 Addresses
        EndpointTestParams{"http://[fe80::1]", true, false},
        EndpointTestParams{"http://[fc00::1]", true, false},
        EndpointTestParams{"https://[fe80::1]", true, true},
        EndpointTestParams{"https://[fc00::1]", true, true},
        // Public IP Addresses
        EndpointTestParams{"http://8.8.8.8", false, false},
        EndpointTestParams{"https://8.8.8.8", true, true},
        EndpointTestParams{"http://1.2.3.4", false, false},
        // Invalid Addresses
        EndpointTestParams{"http://999.999.999.999", false, false},
        EndpointTestParams{"http://invalid-url", false, false},
        // Edge Cases - Boundary IPs
        EndpointTestParams{"http://192.168.0.0", true, false},
        EndpointTestParams{"http://192.168.255.255", true, false},
        EndpointTestParams{"http://172.16.0.0", true, false},
        EndpointTestParams{"http://172.31.255.255", true, false},
        EndpointTestParams{"http://10.0.0.0", true, false},
        EndpointTestParams{"http://10.255.255.255", true, false},
        EndpointTestParams{"http://169.254.0.0", true, false},
        EndpointTestParams{"http://169.254.255.255", true, false},
        EndpointTestParams{"http://[fe80::]", true, false},
        EndpointTestParams{"http://[febf:ffff:ffff:ffff:ffff:ffff:ffff:ffff]",
                           true, false}

        ));

}  // namespace ai_chat
