/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
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
                               LinearModel linear_model)
    : is_initialized_(true) {
  linear_model_ = std::move(linear_model);
  transformations_ = std::move(transformations);
}

void TextProcessing::SetPipeline(PipelineInfo pipeline) {
  version_ = pipeline.version;
  timestamp_ = pipeline.timestamp;
  locale_ = pipeline.locale;
  linear_model_ = std::move(pipeline.linear_model);
  transformations_ = std::move(pipeline.transformations);
}

bool TextProcessing::SetPipeline(base::Value::Dict dict) {
  absl::optional<PipelineInfo> pipeline = ParsePipelineValue(std::move(dict));

  if (pipeline) {
    SetPipeline(std::move(pipeline).value());
    is_initialized_ = true;
  } else {
    is_initialized_ = false;
  }

  return is_initialized_;
}

PredictionMap TextProcessing::Apply(
    const std::unique_ptr<Data>& input_data) const {
  const size_t transformation_count = transformations_.size();

  if (transformation_count == 0) {
    CHECK(input_data->GetType() == DataType::kVector);
    const VectorData* const vector_data =
        static_cast<VectorData*>(input_data.get());
    return linear_model_.GetTopPredictions(*vector_data);
  }

  std::unique_ptr<Data> current_data = transformations_[0]->Apply(input_data);
  for (size_t i = 1; i < transformation_count; ++i) {
    current_data = transformations_[i]->Apply(current_data);
  }

  CHECK(current_data->GetType() == DataType::kVector);
  const VectorData* const vector_data =
      static_cast<VectorData*>(current_data.get());
  return linear_model_.GetTopPredictions(*vector_data);
}

PredictionMap TextProcessing::GetTopPredictions(
    const std::string& content) const {
  std::string stripped_content = StripNonAlphaCharacters(content);
  const PredictionMap predictions =
      Apply(std::make_unique<TextData>(std::move(stripped_content)));
  const double expected_prob =
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

}  // namespace brave_ads::ml::pipeline
