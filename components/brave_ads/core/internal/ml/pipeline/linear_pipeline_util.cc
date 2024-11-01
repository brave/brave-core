/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/linear_pipeline_util.h"

#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_linear_model_generated.h"
#include "brave/components/brave_ads/core/internal/common/resources/flat/text_classification_linear_transformation_generated.h"
#include "brave/components/brave_ads/core/internal/ml/model/linear/linear.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/normalization_transformation.h"
#include "brave/components/l10n/common/locale_util.h"

namespace brave_ads::ml::pipeline {

namespace {

std::optional<TransformationPtr> LoadHashedNGramsTransformation(
    const linear_text_classification::flat::HashedNGramsTransformation*
        hashed_ngram_transformation) {
  if (!hashed_ngram_transformation ||
      !hashed_ngram_transformation->ngrams_range()) {
    return std::nullopt;
  }

  std::vector<uint32_t> subgrams;
  subgrams.reserve(hashed_ngram_transformation->ngrams_range()->size());
  std::ranges::copy(*hashed_ngram_transformation->ngrams_range(),
                    std::back_inserter(subgrams));

  return std::make_unique<HashedNGramsTransformation>(
      hashed_ngram_transformation->num_buckets(), std::move(subgrams));
}

std::optional<TransformationVector> LoadTransformations(
    const linear_text_classification::flat::Model* const text_classification) {
  CHECK(text_classification);

  const auto* const transformations = text_classification->transformations();
  if (!transformations) {
    return std::nullopt;
  }

  TransformationVector transformations_vec;
  for (const auto* const transformation_entry : *transformations) {
    if (!transformation_entry) {
      return std::nullopt;
    }
    std::optional<TransformationPtr> transformation_ptr;
    switch (transformation_entry->transformation_type()) {
      case linear_text_classification::flat::TransformationType::
          TransformationType_LowercaseTransformation: {
        transformation_ptr = std::make_unique<LowercaseTransformation>();
        break;
      }
      case linear_text_classification::flat::TransformationType::
          TransformationType_NormalizeTransformation: {
        transformation_ptr = std::make_unique<NormalizationTransformation>();
        break;
      }
      case linear_text_classification::flat::TransformationType::
          TransformationType_HashedNGramsTransformation: {
        transformation_ptr = LoadHashedNGramsTransformation(
            transformation_entry
                ->transformation_as_HashedNGramsTransformation());
        break;
      }
      case linear_text_classification::flat::TransformationType::
          TransformationType_NONE: {
        NOTREACHED_NORETURN();
      }
    }
    if (!transformation_ptr) {
      return std::nullopt;
    }
    transformations_vec.emplace_back(std::move(*transformation_ptr));
  }

  return transformations_vec;
}

}  // namespace

std::optional<PipelineInfo> LoadLinearPipeline(const uint8_t* const data,
                                               const size_t length) {
  flatbuffers::Verifier verifier(data, length);
  if (!linear_text_classification::flat::VerifyModelBuffer(verifier)) {
    return std::nullopt;
  }

  const linear_text_classification::flat::Model* const model =
      linear_text_classification::flat::GetModel(data);
  if (!model) {
    return std::nullopt;
  }

  const std::string default_language_code =
      brave_l10n::GetISOLanguageCode(brave_l10n::GetDefaultLocaleString());
  const auto* const locale = model->locale();
  if (locale == nullptr ||
      !base::EqualsCaseInsensitiveASCII(locale->str(), default_language_code)) {
    return std::nullopt;
  }

  std::optional<TransformationVector> transformations =
      LoadTransformations(model);
  if (!transformations) {
    return std::nullopt;
  }

  LinearModel linear_model(*model);
  return PipelineInfo(locale->str(), std::move(*transformations),
                      std::move(linear_model), std::nullopt);
}

}  // namespace brave_ads::ml::pipeline
