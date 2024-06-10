/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"

#include <algorithm>
#include <utility>

#include "base/files/file.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/linear_pipeline_util.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/neural_pipeline_util.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"

namespace brave_ads::ml::pipeline {

namespace {

PredictionMap FilterPredictions(const PredictionMap& predictions) {
  const double expected_probability =
      1.0 / std::max(1.0, static_cast<double>(predictions.size()));
  PredictionMap top_predictions;
  for (const auto& [segment, probability] : predictions) {
    if (probability > expected_probability) {
      top_predictions[segment] = probability;
    }
  }
  return top_predictions;
}

}  // namespace

TextProcessing::TextProcessing() = default;

TextProcessing::TextProcessing(TextProcessing&& other) noexcept = default;

TextProcessing& TextProcessing::operator=(TextProcessing&& other) noexcept =
    default;

TextProcessing::~TextProcessing() = default;

TextProcessing::TextProcessing(TransformationVector transformations,
                               std::optional<LinearModel> linear_model)
    : is_initialized_(true) {
  linear_model_ = std::move(linear_model);
  transformations_ = std::move(transformations);
}

base::expected<bool, std::string> TextProcessing::LoadPipeline(
    base::File file) {
  if (!SetPipeline(std::move(file))) {
    return base::unexpected(
        "Failed to load flatbuffers text classification pipeline");
  }

  return IsNeuralPipline();
}

void TextProcessing::SetPipeline(PipelineInfo pipeline) {
  locale_ = pipeline.locale;
  linear_model_ = std::move(pipeline.linear_model);
  neural_model_ = std::move(pipeline.neural_model);
  transformations_ = std::move(pipeline.transformations);
}

bool TextProcessing::SetPipeline(base::File file) {
  std::optional<PipelineInfo> pipeline;

  if (file.IsValid()) {
    pipeline_mapped_file_ = std::make_unique<base::MemoryMappedFile>();
    if (pipeline_mapped_file_->Initialize(std::move(file))) {
      pipeline = LoadNeuralPipeline(pipeline_mapped_file_->data(),
                                    pipeline_mapped_file_->length());
      if (!pipeline) {
        pipeline = LoadLinearPipeline(pipeline_mapped_file_->data(),
                                      pipeline_mapped_file_->length());
      }
    }
  }

  if (pipeline) {
    SetPipeline(std::move(pipeline).value());
    is_initialized_ = true;
  } else {
    pipeline_mapped_file_.reset();
    SetPipeline(PipelineInfo{});
    is_initialized_ = false;
  }

  return is_initialized_;
}

std::optional<PredictionMap> TextProcessing::Predict(
    const VectorData* const vector_data) const {
  if (linear_model_) {
    return linear_model_->GetTopPredictions(*vector_data);
  }
  if (neural_model_) {
    return neural_model_->GetTopPredictions(*vector_data);
  }
  return std::nullopt;
}

std::optional<PredictionMap> TextProcessing::Apply(
    std::unique_ptr<Data> input_data) const {
  std::unique_ptr<Data> mutable_input_data = std::move(input_data);
  CHECK(mutable_input_data);

  const size_t transformation_count = transformations_.size();
  for (size_t i = 0; i < transformation_count; ++i) {
    mutable_input_data = transformations_[i]->Apply(mutable_input_data);
    if (!mutable_input_data) {
      return std::nullopt;
    }
  }
  if (mutable_input_data->GetType() != DataType::kVector) {
    return std::nullopt;
  }

  return Predict(static_cast<VectorData*>(mutable_input_data.get()));
}

std::optional<PredictionMap> TextProcessing::GetPredictions(
    const std::string& text) const {
  std::string stripped_text = StripNonAlphaCharacters(text);
  std::optional<PredictionMap> predictions =
      Apply(std::make_unique<TextData>(std::move(stripped_text)));
  return predictions;
}

std::optional<PredictionMap> TextProcessing::GetTopPredictions(
    const std::string& text) const {
  const std::optional<PredictionMap> predictions = GetPredictions(text);
  if (!predictions) {
    return std::nullopt;
  }
  return FilterPredictions(*predictions);
}

std::optional<PredictionMap> TextProcessing::ClassifyPage(
    const std::string& text) const {
  if (!IsInitialized()) {
    return PredictionMap{};
  }
  return GetTopPredictions(text);
}

}  // namespace brave_ads::ml::pipeline
