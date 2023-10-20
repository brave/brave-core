/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/neural_pipeline_buffer_util.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_neural_model_generated.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_transformation_generated.h"
#include "brave/components/brave_ads/core/internal/ml/model/neural/neural.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/distribution_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation.h"

namespace brave_ads::ml::pipeline {

namespace {

constexpr char kTransformationTypeMappedTokensKey[] = "MAPPED_TOKENS";
constexpr char kTransformationTypeToDistributionKey[] = "TO_DISTRIBUTION";
constexpr char kTransformationTypeToLowerKey[] = "TO_LOWER";

absl::optional<TransformationPtr> ParseSimpleTransformation(
    const text_classification::flat::SimpleTransformation*
        simple_transformation) {
  if (!simple_transformation || !simple_transformation->transformation_type()) {
    return absl::nullopt;
  }

  const std::string transformation_type =
      simple_transformation->transformation_type()->str();

  if (transformation_type == kTransformationTypeToLowerKey) {
    return std::make_unique<LowercaseTransformation>();
  }

  if (transformation_type == kTransformationTypeToDistributionKey) {
    return std::make_unique<DistributionTransformation>();
  }

  return absl::nullopt;
}

absl::optional<TransformationPtr> ParseMappedTokenTransformation(
    const text_classification::flat::MappedTokenTransformation*
        mapped_token_transformation) {
  if (!mapped_token_transformation ||
      !mapped_token_transformation->transformation_type()) {
    return absl::nullopt;
  }

  const std::string transformation_type =
      mapped_token_transformation->transformation_type()->str();

  if (transformation_type != kTransformationTypeMappedTokensKey) {
    return absl::nullopt;
  }

  return std::make_unique<MappedTokensTransformation>(
      mapped_token_transformation);
}

absl::optional<TransformationVector> ParseTransformations(
    const text_classification::flat::NeuralModel* text_classification) {
  CHECK(text_classification);

  const auto* transformations = text_classification->transformations();
  if (!transformations) {
    return absl::nullopt;
  }

  TransformationVector transformations_vec;
  for (const auto* transformation_entry : *transformations) {
    if (!transformation_entry) {
      return absl::nullopt;
    }
    absl::optional<TransformationPtr> transformation_ptr;
    switch (transformation_entry->transformation_type()) {
      case text_classification::flat::Transformation::
          Transformation_SimpleTransformation:
      case text_classification::flat::Transformation::
          Transformation_HashedNGramsTransformation: {
        transformation_ptr = ParseSimpleTransformation(
            transformation_entry->transformation_as_SimpleTransformation());
        break;
      }
      case text_classification::flat::Transformation::
          Transformation_MappedTokenTransformation: {
        transformation_ptr = ParseMappedTokenTransformation(
            transformation_entry
                ->transformation_as_MappedTokenTransformation());
        break;
      }
      case text_classification::flat::Transformation::Transformation_NONE: {
        NOTREACHED_NORETURN();
      }
    }
    if (!transformation_ptr) {
      return absl::nullopt;
    }
    transformations_vec.emplace_back(std::move(*transformation_ptr));
  }

  return transformations_vec;
}

}  // namespace

absl::optional<PipelineInfo> ParseNeuralPipelineBuffer(const std::string& buffer) {
  flatbuffers::Verifier verifier(
      reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size());
  if (!text_classification::flat::VerifyNeuralModelBuffer(verifier)) {
    return absl::nullopt;
  }

  const text_classification::flat::NeuralModel* model =
      text_classification::flat::GetNeuralModel(buffer.data());
  if (!model) {
    return absl::nullopt;
  }

  NeuralModel neural_model = NeuralModel(model);

  absl::optional<TransformationVector> transformations =
      ParseTransformations(model);
  if (!transformations) {
    return absl::nullopt;
  }

  const auto* locale = model->locale();
  if (!locale) {
    return absl::nullopt;
  }

  return PipelineInfo(locale->str(), std::move(*transformations), absl::nullopt,
                      std::move(neural_model));
}

}  // namespace brave_ads::ml::pipeline
