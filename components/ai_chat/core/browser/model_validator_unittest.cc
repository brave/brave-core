// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/model_validator.h"

#include <limits>
#include <optional>
#include <string>
#include <utility>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

// Test fixture class for ModelValidator
class ModelValidatorUnitTest : public ::testing::Test {};

// Test IsValidContextSize with various context sizes
TEST_F(ModelValidatorUnitTest, IsValidContextSize) {
  struct TestCase {
    std::optional<int32_t> context_size;
    bool expected_result;
  };

  TestCase test_cases[] = {
      {1000, true},
      {1, true},              // Minimum valid size
      {std::nullopt, false},  // Nullopt should return false
      {0, false},             // Invalid size (less than MIN_CONTEXT_SIZE)
      {static_cast<int32_t>(kMaxCustomModelContextSize),
       true},  // Max valid size
      {static_cast<int32_t>(kMaxCustomModelContextSize) + 1,
       false},                                       // Beyond max size
      {std::numeric_limits<int32_t>::max(), false},  // Beyond valid size limit
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

// Test IsValidEndpoint with various URLs
TEST_F(ModelValidatorUnitTest, IsValidEndpoint) {
  struct TestCase {
    std::string url;
    bool expected_result;
  };

  TestCase test_cases[] = {
      {"https://valid-url.com", true},    // Valid HTTPS URL
      {"http://invalid-url.com", false},  // HTTP URL (invalid)
      {"https://localhost:8080", true},   // Valid localhost URL
      {"invalid-url", false},             // Invalid URL string
      {"https://", false},                // Incomplete URL
      {"https://search.brave.com/search?q=foo",
       true},  // Valid Brave search URL
  };

  for (const auto& test_case : test_cases) {
    GURL endpoint(test_case.url);
    bool actual = ModelValidator::IsValidEndpoint(endpoint);
    EXPECT_EQ(actual, test_case.expected_result)
        << "Failed for URL: " << test_case.url;
  }
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

  EXPECT_EQ(ModelValidator::ValidateModel(valid_custom_model),
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

  EXPECT_EQ(ModelValidator::ValidateModel(invalid_context_size_model),
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

  EXPECT_EQ(ModelValidator::ValidateModel(invalid_endpoint_model),
            ModelValidationResult::kInvalidUrl);

  // Nullopt for context size should fail
  mojom::CustomModelOptionsPtr nullopt_context_size_options =
      mojom::CustomModelOptions::New();
  nullopt_context_size_options->context_size = std::nullopt;
  nullopt_context_size_options->endpoint = GURL("https://valid-url.com");

  mojom::Model nullopt_context_size_model;
  nullopt_context_size_model.options =
      mojom::ModelOptions::NewCustomModelOptions(
          std::move(nullopt_context_size_options));

  EXPECT_EQ(ModelValidator::ValidateModel(nullopt_context_size_model),
            ModelValidationResult::kInvalidContextSize);
}

}  // namespace ai_chat
