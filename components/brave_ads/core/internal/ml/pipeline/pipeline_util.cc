/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_util.h"

#include <cstddef>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/distribution_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/normalization_transformation.h"

namespace brave_ads::ml::pipeline {

namespace {

constexpr char kBiasesKey[] = "biases";
constexpr char kClassesKey[] = "classes";

constexpr char kClassifierKey[] = "classifier";
constexpr char kClassifierTypeKey[] = "classifier_type";
constexpr char kClassifierTypeLinearKey[] = "LINEAR";

constexpr char kClassWeightsKey[] = "class_weights";
constexpr char kLocaleKey[] = "locale";
constexpr char kNumberBucketsKey[] = "num_buckets";

constexpr char kNgramsRangeKey[] = "ngrams_range";
constexpr char kParamsKey[] = "params";

constexpr char kTransformationsKey[] = "transformations";
constexpr char kTransformationTypeKey[] = "transformation_type";
constexpr char kTransformationTypeHashedNgramsKey[] = "HASHED_NGRAMS";
constexpr char kTransformationTypeNormalizeKey[] = "NORMALIZE";
constexpr char kTransformationTypeToDistributionKey[] = "TO_DISTRIBUTION";
constexpr char kTransformationTypeToLowerKey[] = "TO_LOWER";

std::vector<int> FillSubgrams(const base::Value::List* ngrams_range) {
  std::vector<int> subgrams;
  for (const base::Value& subgram : *ngrams_range) {
    if (!subgram.is_int()) {
      return {};
    }
    subgrams.push_back(subgram.GetInt());
  }
  return subgrams;
}

absl::optional<TransformationPtr> ParsePipelineTransformationHashedNgrams(
    const base::Value::Dict* transformation_dict) {
  const auto* const transformation_params =
      transformation_dict->FindDict(kParamsKey);
  if (!transformation_params) {
    return absl::nullopt;
  }

  const absl::optional<int> num_buckets =
      transformation_params->FindInt(kNumberBucketsKey);
  if (!num_buckets) {
    return absl::nullopt;
  }
  const auto* const ngrams_range =
      transformation_params->FindList(kNgramsRangeKey);
  if (!ngrams_range) {
    return absl::nullopt;
  }
  std::vector<int> subgrams = FillSubgrams(ngrams_range);
  if (subgrams.empty()) {
    return absl::nullopt;
  }
  return std::make_unique<HashedNGramsTransformation>(*num_buckets, subgrams);
}

absl::optional<TransformationPtr> AddPipelineTransformation(
    const std::string& transformation_type,
    const base::Value::Dict* transformation_dict) {
  if (transformation_type == kTransformationTypeToLowerKey) {
    return std::make_unique<LowercaseTransformation>();
  }

  if (transformation_type == kTransformationTypeNormalizeKey) {
    return std::make_unique<NormalizationTransformation>();
  }

  if (transformation_type == kTransformationTypeHashedNgramsKey) {
    absl::optional<TransformationPtr> hashed_ngrams_transformation =
        ParsePipelineTransformationHashedNgrams(transformation_dict);
    if (!hashed_ngrams_transformation) {
      return absl::nullopt;
    }
    return std::move(*hashed_ngrams_transformation);
  }

  if (transformation_type == kTransformationTypeToDistributionKey) {
    return std::make_unique<DistributionTransformation>();
  }

  return absl::nullopt;
}

TransformationVector ParsePipelineTransformations(
    const base::Value::List& transformations) {
  TransformationVector transformation_vector;

  for (const auto& transformation : transformations) {
    const auto* const transformation_dict = transformation.GetIfDict();
    if (!transformation_dict) {
      return {};
    }

    const std::string* const transformation_type =
        transformation_dict->FindString(kTransformationTypeKey);
    if (!transformation_type) {
      return {};
    }
    absl::optional<TransformationPtr> next_transformation =
        AddPipelineTransformation(*transformation_type, transformation_dict);
    if (!next_transformation) {
      return {};
    }
    transformation_vector.push_back(std::move(*next_transformation));
  }
  return transformation_vector;
}

std::vector<std::string> ParsePipelineClassifierClasses(
    const base::Value::Dict& classifier) {
  const auto* const classifier_classes = classifier.FindList(kClassesKey);
  if (!classifier_classes) {
    return {};
  }

  std::vector<std::string> classes;
  classes.reserve(classifier_classes->size());
  for (const base::Value& classifier_class : *classifier_classes) {
    if (!classifier_class.is_string()) {
      return {};
    }
    const std::string& class_name = classifier_class.GetString();
    if (class_name.empty()) {
      return {};
    }
    classes.push_back(class_name);
  }
  return classes;
}

std::map<std::string, VectorData> FillClassWeights(
    const base::Value::Dict* class_weights,
    const std::vector<std::string>& classes_names) {
  CHECK(class_weights);

  std::map</*class_name=*/std::string, /*weights=*/VectorData>
      filled_class_weights;

  for (const std::string& class_name : classes_names) {
    const auto* const weights = class_weights->FindList(class_name);
    if (!weights) {
      return {};
    }
    std::vector<float> class_coefficient_weights;
    class_coefficient_weights.reserve(weights->size());
    for (const base::Value& weight : *weights) {
      if (!weight.is_double() && !weight.is_int()) {
        return {};
      }
      class_coefficient_weights.push_back(
          static_cast<float>(weight.GetDouble()));
    }
    filled_class_weights[class_name] =
        VectorData(std::move(class_coefficient_weights));
  }
  return filled_class_weights;
}

std::map<std::string, VectorData> ParsePipelineClassifierWeights(
    const base::Value::Dict& classifier,
    const std::vector<std::string>& classes) {
  const auto* const class_weights = classifier.FindDict(kClassWeightsKey);
  if (!class_weights) {
    return {};
  }

  std::map<std::string, VectorData> filled_class_weights =
      FillClassWeights(class_weights, classes);
  if (filled_class_weights.empty()) {
    return {};
  }
  return filled_class_weights;
}

std::map<std::string, double> FillBiases(
    const base::Value::List* biases,
    const std::vector<std::string>& classes) {
  CHECK(biases);

  std::map</*class_name=*/std::string, /*bias=*/double> filled_biases;

  for (size_t i = 0; i < biases->size(); ++i) {
    const base::Value& bias = (*biases)[i];
    if (!bias.is_double() && !bias.is_int()) {
      return {};
    }
    filled_biases[classes[i]] = bias.GetDouble();
  }
  return filled_biases;
}

std::map<std::string, double> ParsePipelineClassifierBiases(
    const base::Value::Dict& classifier,
    const std::vector<std::string>& classes) {
  const auto* const biases = classifier.FindList(kBiasesKey);
  if (!biases || biases->size() != classes.size()) {
    return {};
  }

  std::map<std::string, double> filled_biases = FillBiases(biases, classes);
  if (filled_biases.empty()) {
    return {};
  }
  return filled_biases;
}

absl::optional<LinearModel> ParsePipelineClassifierLinear(
    const base::Value::Dict& classifier) {
  std::vector<std::string> classes = ParsePipelineClassifierClasses(classifier);
  if (classes.empty()) {
    return absl::nullopt;
  }
  std::map<std::string, VectorData> class_weights =
      ParsePipelineClassifierWeights(classifier, classes);
  if (class_weights.empty()) {
    return absl::nullopt;
  }
  std::map<std::string, double> biases =
      ParsePipelineClassifierBiases(classifier, classes);
  if (biases.empty()) {
    return absl::nullopt;
  }
  return LinearModel(std::move(class_weights), std::move(biases));
}

std::string ParsePipelineValueLocale(base::Value::Dict& dict) {
  const std::string* const locale = dict.FindString(kLocaleKey);
  if (!locale) {
    return {};
  }
  return *locale;
}

TransformationVector ParsePipelineValueTransformations(
    base::Value::Dict& dict) {
  const auto* transformations = dict.FindList(kTransformationsKey);
  if (!transformations) {
    return {};
  }
  TransformationVector transformation_vector =
      ParsePipelineTransformations(*transformations);
  if (transformation_vector.empty()) {
    return {};
  }
  return transformation_vector;
}

std::string ParsePipelineValueClassifierType(base::Value::Dict& dict) {
  const auto* classifier = dict.FindDict(kClassifierKey);
  if (!classifier) {
    return {};
  }
  const std::string* const classifier_type =
      classifier->FindString(kClassifierTypeKey);
  if (!classifier_type) {
    return {};
  }
  return *classifier_type;
}

absl::optional<LinearModel> ParsePipelineValueClassifierLinear(
    base::Value::Dict& dict,
    const std::string& classifier_type) {
  if (classifier_type != kClassifierTypeLinearKey) {
    return absl::nullopt;
  }

  const auto* classifier = dict.FindDict(kClassifierKey);
  if (!classifier) {
    return absl::nullopt;
  }
  return ParsePipelineClassifierLinear(*classifier);
}

}  // namespace

absl::optional<PipelineInfo> ParsePipelineValue(base::Value::Dict dict) {
  const std::string locale = ParsePipelineValueLocale(dict);
  if (locale.empty()) {
    return absl::nullopt;
  }

  TransformationVector transformations =
      ParsePipelineValueTransformations(dict);
  if (transformations.empty()) {
    return absl::nullopt;
  }

  const std::string classifier_type = ParsePipelineValueClassifierType(dict);
  absl::optional<LinearModel> linear_model =
      ParsePipelineValueClassifierLinear(dict, classifier_type);
  if (!linear_model) {
    return absl::nullopt;
  }

  return PipelineInfo(locale, std::move(transformations),
                      std::move(linear_model), absl::nullopt);
}

}  // namespace brave_ads::ml::pipeline
