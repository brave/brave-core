/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/neural_pipeline_util.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_neural_model_generated.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_neural_transformation_generated.h"
#include "brave/components/brave_ads/core/internal/ml/model/neural/neural.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/distribution_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation.h"
#include "brave/components/l10n/common/locale_util.h"

namespace brave_ads::ml::pipeline {

namespace {

absl::optional<TransformationPtr> LoadMappedTokenTransformation(
    const neural_text_classification::flat::MappedTokenTransformation*
        mapped_token_transformation) {
  if (!mapped_token_transformation) {
    return absl::nullopt;
  }

  return std::make_unique<MappedTokensTransformation>(
      mapped_token_transformation);
}

absl::optional<TransformationVector> LoadTransformations(
    const neural_text_classification::flat::Model* text_classification) {
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
      case neural_text_classification::flat::TransformationType::
          TransformationType_LowercaseTransformation: {
        transformation_ptr = std::make_unique<LowercaseTransformation>();
        break;
      }
      case neural_text_classification::flat::TransformationType::
          TransformationType_DistributionTransformation: {
        transformation_ptr = std::make_unique<DistributionTransformation>();
        break;
      }
      case neural_text_classification::flat::TransformationType::
          TransformationType_MappedTokenTransformation: {
        transformation_ptr = LoadMappedTokenTransformation(
            transformation_entry
                ->transformation_as_MappedTokenTransformation());
        break;
      }
      case neural_text_classification::flat::TransformationType::
          TransformationType_NONE: {
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

absl::optional<PipelineInfo> LoadNeuralPipeline(const std::string& buffer) {
  flatbuffers::Verifier verifier(
      reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size());
  if (!neural_text_classification::flat::VerifyModelBuffer(verifier)) {
    return absl::nullopt;
  }

  const neural_text_classification::flat::Model* model =
      neural_text_classification::flat::GetModel(buffer.data());
  if (!model) {
    return absl::nullopt;
  }

  const std::string default_language_code =
      brave_l10n::GetISOLanguageCode(brave_l10n::GetDefaultLocaleString());
  const auto* locale = model->locale();
  if (!locale ||
      !base::EqualsCaseInsensitiveASCII(locale->str(), default_language_code)) {
    return absl::nullopt;
  }

  absl::optional<TransformationVector> transformations =
      LoadTransformations(model);
  if (!transformations) {
    return absl::nullopt;
  }

  NeuralModel neural_model(model);
  return PipelineInfo(locale->str(), std::move(*transformations), absl::nullopt,
                      std::move(neural_model));
}

}  // namespace brave_ads::ml::pipeline
