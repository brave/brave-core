/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"

#include <algorithm>
#include <utility>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::ml::pipeline {

// static
base::expected<TextProcessing, std::string> TextProcessing::CreateFromValue(
    base::Value::Dict dict) {
  TextProcessing text_processing;
  if (!text_processing.SetPipeline(std::move(dict))) {
    return base::unexpected(
        "Failed to parse text classification pipeline JSON");
  }
  return text_processing;
}

TextProcessing::TextProcessing() = default;

TextProcessing::TextProcessing(TextProcessing&& other) noexcept = default;

TextProcessing& TextProcessing::operator=(TextProcessing&& other) noexcept =
    default;

TextProcessing::~TextProcessing() = default;

TextProcessing::TextProcessing(TransformationVector transformations,
                               absl::optional<LinearModel> linear_model)
    : is_initialized_(true) {
  linear_model_ = std::move(linear_model);
  transformations_ = std::move(transformations);
}

void TextProcessing::SetPipeline(PipelineInfo pipeline) {
  version_ = pipeline.version;
  timestamp_ = pipeline.timestamp;
  locale_ = pipeline.locale;
  linear_model_ = std::move(pipeline.linear_model);
  neural_model_ = std::move(pipeline.neural_model);
  transformations_ = std::move(pipeline.transformations);
}

bool TextProcessing::SetPipeline(base::Value::Dict dict) {
  absl::optional<PipelineInfo> pipeline = ParsePipelineValue(std::move(dict));

  if (pipeline) {
    SetPipeline(std::move(pipeline).value());
    is_initialized_ = true;
  } else {
    SetPipeline(PipelineInfo{});
    is_initialized_ = false;
  }

  return is_initialized_;
}

absl::optional<PredictionMap> TextProcessing::Predict(
    VectorData* vector_data) const {
  if (linear_model_) {
    return linear_model_->GetTopPredictions(*vector_data);
  }
  if (neural_model_) {
    return neural_model_->GetTopPredictions(*vector_data);
  }
  return absl::nullopt;
}

absl::optional<PredictionMap> TextProcessing::Apply(
    std::unique_ptr<Data> input_data) const {
  std::unique_ptr<Data> mutable_input_data = std::move(input_data);
  CHECK(mutable_input_data);

  const size_t transformation_count = transformations_.size();
  for (size_t i = 0; i < transformation_count; ++i) {
    mutable_input_data = transformations_[i]->Apply(mutable_input_data);
    if (!mutable_input_data) {
      BLOG(0, "TextProcessing transformation failed due to an invalid model");
      return absl::nullopt;
    }
  }
  if (mutable_input_data->GetType() != DataType::kVector) {
    BLOG(0, "Predictions failed due to an invalid model");
    return absl::nullopt;
  }

  return Predict(static_cast<VectorData*>(mutable_input_data.get()));
}

absl::optional<PredictionMap> TextProcessing::GetPredictions(
    const std::string& text) const {
  std::string stripped_text = StripNonAlphaCharacters(text);
  absl::optional<PredictionMap> predictions =
      Apply(std::make_unique<TextData>(std::move(stripped_text)));
  return predictions;
}

// static
PredictionMap TextProcessing::FilterPredictions(
    const PredictionMap& predictions) {
  const double expected_probability =
      1.0 / std::max(1.0, static_cast<double>(predictions.size()));
  PredictionMap top_predictions;
  for (const auto& [segment, probability] : predictions) {
    if (probability > expected_probability) {
      top_predictions[segment] = probability;
      BLOG(6, segment);
      BLOG(6, probability);
    }
  }
  return top_predictions;
}

absl::optional<PredictionMap> TextProcessing::GetTopPredictions(
    const std::string& text) const {
  const absl::optional<PredictionMap> predictions = GetPredictions(text);
  if (!predictions) {
    return absl::nullopt;
  }
  PredictionMap top_predictions = FilterPredictions(*predictions);
  return top_predictions;
}

absl::optional<PredictionMap> TextProcessing::ClassifyPage(
    const std::string& text) const {
  if (!IsInitialized()) {
    return PredictionMap{};
  }
  return GetTopPredictions(text);
}

}  // namespace brave_ads::ml::pipeline
