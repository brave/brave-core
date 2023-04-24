/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_util.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/normalization_transformation.h"

namespace brave_ads::ml::pipeline {

namespace {

// TODO(https://github.com/brave/brave-browser/issues/24940): Reduce cognitive
// complexity.
absl::optional<TransformationVector> ParsePipelineTransformations(
    base::Value::List* transformations_value) {
  if (!transformations_value) {
    return absl::nullopt;
  }

  absl::optional<TransformationVector> transformations = TransformationVector();
  for (const base::Value& item : *transformations_value) {
    const base::Value::Dict& transformation = item.GetDict();
    const std::string* const transformation_type =
        transformation.FindString("transformation_type");

    if (!transformation_type) {
      return absl::nullopt;
    }

    const std::string parsed_transformation_type = *transformation_type;

    if (parsed_transformation_type == "TO_LOWER") {
      transformations->push_back(std::make_unique<LowercaseTransformation>());
    }

    if (parsed_transformation_type == "NORMALIZE") {
      transformations->push_back(
          std::make_unique<NormalizationTransformation>());
    }

    if (parsed_transformation_type == "HASHED_NGRAMS") {
      const base::Value::Dict* const transformation_params =
          transformation.FindDict("params");

      if (!transformation_params) {
        return absl::nullopt;
      }

      const absl::optional<int> nb =
          transformation_params->FindInt("num_buckets");
      if (!nb) {
        return absl::nullopt;
      }
      const int num_buckets = *nb;

      const base::Value::List* const ngram_sizes =
          transformation_params->FindList("ngrams_range");
      if (!ngram_sizes) {
        return absl::nullopt;
      }

      std::vector<int> ngram_range;
      for (const base::Value& n : *ngram_sizes) {
        if (n.is_int()) {
          ngram_range.push_back(n.GetInt());
        } else {
          return absl::nullopt;
        }
      }
      transformations->push_back(std::make_unique<HashedNGramsTransformation>(
          num_buckets, ngram_range));
    }
  }

  return transformations;
}

// TODO(https://github.com/brave/brave-browser/issues/24941): Reduce cognitive
// complexity.
absl::optional<LinearModel> ParsePipelineClassifier(
    base::Value::Dict* classifier_value) {
  if (!classifier_value) {
    return absl::nullopt;
  }

  const std::string* const classifier_type =
      classifier_value->FindString("classifier_type");

  if (!classifier_type) {
    return absl::nullopt;
  }

  const std::string parsed_classifier_type = *classifier_type;

  if (parsed_classifier_type != "LINEAR") {
    return absl::nullopt;
  }

  base::Value::List* specified_classes = classifier_value->FindList("classes");
  if (!specified_classes) {
    return absl::nullopt;
  }

  std::vector<std::string> classes;
  classes.reserve(specified_classes->size());
  for (const base::Value& class_name : *specified_classes) {
    if (!class_name.is_string()) {
      return absl::nullopt;
    }

    const std::string& class_string = class_name.GetString();
    if (class_string.empty()) {
      return absl::nullopt;
    }

    classes.push_back(class_string);
  }

  base::Value::Dict* class_weights =
      classifier_value->FindDict("class_weights");
  if (!class_weights) {
    return absl::nullopt;
  }

  std::map<std::string, VectorData> weights;
  for (const std::string& class_string : classes) {
    base::Value::List* list = class_weights->FindList(class_string);
    if (!list) {
      return absl::nullopt;
    }

    std::vector<float> class_coef_weights;
    class_coef_weights.reserve(list->size());
    for (const base::Value& weight : *list) {
      if (weight.is_double() || weight.is_int()) {
        class_coef_weights.push_back(weight.GetDouble());
      } else {
        return absl::nullopt;
      }
    }
    weights[class_string] = VectorData(std::move(class_coef_weights));
  }

  std::map<std::string, double> specified_biases;
  base::Value::List* biases = classifier_value->FindList("biases");
  if (!biases) {
    return absl::nullopt;
  }

  if (biases->size() != classes.size()) {
    return absl::nullopt;
  }

  for (size_t i = 0; i < biases->size(); i++) {
    const base::Value& this_bias = (*biases)[i];
    if (this_bias.is_double() || this_bias.is_int()) {
      specified_biases[classes[i]] = this_bias.GetDouble();
    } else {
      return absl::nullopt;
    }
  }

  return LinearModel(std::move(weights), std::move(specified_biases));
}

}  // namespace

absl::optional<PipelineInfo> ParsePipelineValue(base::Value::Dict dict) {
  const absl::optional<int> version = dict.FindInt("version");
  if (!version) {
    return absl::nullopt;
  }

  const std::string* const timestamp = dict.FindString("timestamp");
  if (!timestamp) {
    return absl::nullopt;
  }

  const std::string* const locale = dict.FindString("locale");
  if (!locale) {
    return absl::nullopt;
  }

  absl::optional<TransformationVector> transformations =
      ParsePipelineTransformations(dict.FindList("transformations"));
  if (!transformations) {
    return absl::nullopt;
  }

  absl::optional<LinearModel> linear_model =
      ParsePipelineClassifier(dict.FindDict("classifier"));
  if (!linear_model) {
    return absl::nullopt;
  }

  return PipelineInfo(*version, *timestamp, *locale,
                      std::move(*transformations), std::move(*linear_model));
}

}  // namespace brave_ads::ml::pipeline
