/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/text_processing/text_processing.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/values.h"
#include "bat/ads/internal/base/strings/string_strip_util.h"
#include "bat/ads/internal/ml/data/text_data.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/pipeline/pipeline_info.h"
#include "bat/ads/internal/ml/pipeline/pipeline_util.h"
#include "bat/ads/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "bat/ads/internal/ml/transformation/lowercase_transformation.h"
#include "bat/ads/internal/ml/transformation/normalization_transformation.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace ml {
namespace pipeline {

// static
std::unique_ptr<TextProcessing> TextProcessing::CreateFromValue(
    base::Value resource_value,
    std::string* error_message) {
  DCHECK(error_message);

  auto text_processing = std::make_unique<TextProcessing>();
  if (!text_processing->FromValue(std::move(resource_value))) {
    *error_message = "Failed to parse text classification pipeline JSON";
    return {};
  }

  return text_processing;
}

bool TextProcessing::IsInitialized() const {
  return is_initialized_;
}

TextProcessing::TextProcessing() = default;

TextProcessing::~TextProcessing() = default;

TextProcessing::TextProcessing(TransformationVector transformations,
                               model::Linear linear_model)
    : is_initialized_(true) {
  linear_model_ = std::move(linear_model);
  transformations_ = std::move(transformations);
}

void TextProcessing::SetInfo(PipelineInfo info) {
  version_ = info.version;
  timestamp_ = info.timestamp;
  locale_ = info.locale;
  linear_model_ = std::move(info.linear_model);
  transformations_ = std::move(info.transformations);
}

bool TextProcessing::FromValue(base::Value value) {
  absl::optional<PipelineInfo> pipeline = ParsePipelineValue(std::move(value));

  if (pipeline) {
    SetInfo(std::move(*pipeline));
    is_initialized_ = true;
  } else {
    is_initialized_ = false;
  }

  return is_initialized_;
}

PredictionMap TextProcessing::Apply(
    const std::unique_ptr<Data>& input_data) const {
  const size_t transformation_count = transformations_.size();

  if (!transformation_count) {
    DCHECK(input_data->GetType() == DataType::kVector);
    const VectorData* vector_data = static_cast<VectorData*>(input_data.get());
    return linear_model_.GetTopPredictions(*vector_data);
  }

  std::unique_ptr<Data> current_data = transformations_[0]->Apply(input_data);
  for (size_t i = 1; i < transformation_count; ++i) {
    current_data = transformations_[i]->Apply(current_data);
  }

  DCHECK(current_data->GetType() == DataType::kVector);
  const VectorData* vector_data = static_cast<VectorData*>(current_data.get());
  return linear_model_.GetTopPredictions(*vector_data);
}

PredictionMap TextProcessing::GetTopPredictions(
    const std::string& content) const {
  std::string stripped_content = StripNonAlphaCharacters(content);
  PredictionMap predictions =
      Apply(std::make_unique<TextData>(std::move(stripped_content)));
  double expected_prob =
      1.0 / std::max(1.0, static_cast<double>(predictions.size()));
  PredictionMap rtn;
  for (auto const& prediction : predictions) {
    if (prediction.second > expected_prob) {
      rtn[prediction.first] = prediction.second;
    }
  }
  return rtn;
}

PredictionMap TextProcessing::ClassifyPage(const std::string& content) const {
  if (!IsInitialized()) {
    return {};
  }

  return GetTopPredictions(content);
}

}  // namespace pipeline
}  // namespace ml
}  // namespace ads
