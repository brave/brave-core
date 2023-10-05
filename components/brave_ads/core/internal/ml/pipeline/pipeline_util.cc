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
    const base::Value::List& list) {
  TransformationVector transformation_vector;

  for (const auto& item : list) {
    const auto* const item_dict = item.GetIfDict();
    if (!item_dict) {
      return absl::nullopt;
    }

    const std::string* const transformation_type =
        item_dict->FindString("transformation_type");
    if (!transformation_type) {
      return absl::nullopt;
    }

    if (*transformation_type == "TO_LOWER") {
      transformation_vector.push_back(
          std::make_unique<LowercaseTransformation>());
    }

    if (*transformation_type == "NORMALIZE") {
      transformation_vector.push_back(
          std::make_unique<NormalizationTransformation>());
    }

    if (*transformation_type == "HASHED_NGRAMS") {
      const auto* const params_dict = item_dict->FindDict("params");
      if (!params_dict) {
        return absl::nullopt;
      }

      const absl::optional<int> num_buckets =
          params_dict->FindInt("num_buckets");
      if (!num_buckets) {
        return absl::nullopt;
      }

      const auto* const ngrams_range_list =
          params_dict->FindList("ngrams_range");
      if (!ngrams_range_list) {
        return absl::nullopt;
      }

      std::vector<int> subgrams;
      for (const base::Value& subgram : *ngrams_range_list) {
        if (!subgram.is_int()) {
          return absl::nullopt;
        }

        subgrams.push_back(subgram.GetInt());
      }

      transformation_vector.push_back(
          std::make_unique<HashedNGramsTransformation>(*num_buckets, subgrams));
    }
  }

  return transformation_vector;
}

// TODO(https://github.com/brave/brave-browser/issues/24941): Reduce cognitive
// complexity.
absl::optional<LinearModel> ParsePipelineClassifier(
    const base::Value::Dict& dict) {
  const std::string* const classifier_type = dict.FindString("classifier_type");
  if (!classifier_type || *classifier_type != "LINEAR") {
    return absl::nullopt;
  }

  const auto* const classes_list = dict.FindList("classes");
  if (!classes_list) {
    return absl::nullopt;
  }

  std::vector<std::string> classes;
  classes.reserve(classes_list->size());
  for (const base::Value& item : *classes_list) {
    if (!item.is_string()) {
      return absl::nullopt;
    }

    const std::string& class_name = item.GetString();
    if (class_name.empty()) {
      return absl::nullopt;
    }

    classes.push_back(class_name);
  }

  const auto* const class_weights_dict = dict.FindDict("class_weights");
  if (!class_weights_dict) {
    return absl::nullopt;
  }

  std::map</*class_name*/ std::string, /*weights*/ VectorData> class_weights;
  for (const std::string& class_name : classes) {
    const auto* const list = class_weights_dict->FindList(class_name);
    if (!list) {
      return absl::nullopt;
    }

    std::vector<float> class_coef_weights;
    class_coef_weights.reserve(list->size());
    for (const base::Value& item : *list) {
      if (!item.is_double() && !item.is_int()) {
        return absl::nullopt;
      }

      class_coef_weights.push_back(static_cast<float>(item.GetDouble()));
    }

    class_weights[class_name] = VectorData(std::move(class_coef_weights));
  }

  const auto* const biases_list = dict.FindList("biases");
  if (!biases_list || biases_list->size() != classes.size()) {
    return absl::nullopt;
  }

  std::map</*class_name*/ std::string, /*bias*/ double> biases;
  for (size_t i = 0; i < biases_list->size(); ++i) {
    const base::Value& bias = (*biases_list)[i];
    if (!bias.is_double() && !bias.is_int()) {
      return absl::nullopt;
    }

    biases[classes[i]] = bias.GetDouble();
  }

  return LinearModel(std::move(class_weights), std::move(biases));
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

  const auto* transformations_list = dict.FindList("transformations");
  if (!transformations_list) {
    return absl::nullopt;
  }
  absl::optional<TransformationVector> transformations =
      ParsePipelineTransformations(*transformations_list);
  if (!transformations) {
    return absl::nullopt;
  }

  const auto* classifier_dict = dict.FindDict("classifier");
  if (!classifier_dict) {
    return absl::nullopt;
  }
  absl::optional<LinearModel> linear_model =
      ParsePipelineClassifier(*classifier_dict);
  if (!linear_model) {
    return absl::nullopt;
  }

  return PipelineInfo(*version, *timestamp, *locale,
                      std::move(*transformations), std::move(*linear_model));
}

}  // namespace brave_ads::ml::pipeline
