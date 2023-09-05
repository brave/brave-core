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

#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/distribution_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/normalization_transformation.h"

namespace brave_ads::ml::pipeline {

namespace {

absl::optional<TransformationPtr> ParsePipelineTransformationHashedNgrams(
    const base::Value::Dict* item_dict) {
  const auto* const params_dict = item_dict->FindDict("params");
  if (!params_dict) {
    return absl::nullopt;
  }

  const absl::optional<int> num_buckets = params_dict->FindInt("num_buckets");
  if (!num_buckets) {
    return absl::nullopt;
  }

  const auto* const ngrams_range_list = params_dict->FindList("ngrams_range");
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

  return std::make_unique<HashedNGramsTransformation>(*num_buckets, subgrams);
}

absl::optional<TransformationPtr> ParsePipelineTransformationMappedTokens(
    const auto* const item_dict) {
  const absl::optional<int> dimension = item_dict->FindInt("dimension");
  if (!dimension) {
    return absl::nullopt;
  }

  const auto* const token_categories_mapping_dict =
      item_dict->FindDict("mapping");
  if (!token_categories_mapping_dict) {
    return absl::nullopt;
  }

  int vector_dimension = *dimension;
  std::map<std::string, std::vector<int>> token_categories_mapping;

  for (const auto [token, mapping] : *token_categories_mapping_dict) {
    const auto* mapping_list = mapping.GetIfList();
    if (!mapping_list) {
      continue;
    }

    std::string token_text = token;
    std::vector<int> mapped_categories;
    mapped_categories.reserve(mapping_list->size());
    for (const base::Value& mapping_index : *mapping_list) {
      if (!mapping_index.is_double() && !mapping_index.is_int()) {
        return absl::nullopt;
      }

      mapped_categories.push_back(static_cast<int>(mapping_index.GetInt()));
    }

    token_categories_mapping[token_text] = mapped_categories;
  }

  return std::make_unique<MappedTokensTransformation>(vector_dimension,
                                                      token_categories_mapping);
}

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
      absl::optional<TransformationPtr> hashed_ngrams_transformation =
          ParsePipelineTransformationHashedNgrams(item_dict);
      if (!hashed_ngrams_transformation) {
        return absl::nullopt;
      }
      transformation_vector.push_back(std::move(*hashed_ngrams_transformation));
    }

    if (*transformation_type == "MAPPED_TOKENS") {
      absl::optional<TransformationPtr> mapped_tokens_transformation =
          ParsePipelineTransformationMappedTokens(item_dict);
      if (!mapped_tokens_transformation) {
        return absl::nullopt;
      }
      transformation_vector.push_back(std::move(*mapped_tokens_transformation));
    }

    if (*transformation_type == "TO_DISTRIBUTION") {
      transformation_vector.push_back(
          std::make_unique<DistributionTransformation>());
    }
  }

  return transformation_vector;
}

// TODO(https://github.com/brave/brave-browser/issues/24941): Reduce
// cognitive complexity.
std::vector<std::string> ParsePipelineClassifierClasses(
    const base::Value::Dict& dict) {
  const auto* const classes_list = dict.FindList("classes");
  if (!classes_list) {
    return {};
  }

  std::vector<std::string> classes;
  classes.reserve(classes_list->size());
  for (const base::Value& item : *classes_list) {
    if (!item.is_string()) {
      return {};
    }

    const std::string& class_name = item.GetString();
    if (class_name.empty()) {
      return {};
    }

    classes.push_back(class_name);
  }
  return classes;
}

std::map<std::string, VectorData> ParsePipelineClassifierWeights(
    const base::Value::Dict& dict,
    const std::vector<std::string>& classes) {
  const auto* const class_weights_dict = dict.FindDict("class_weights");
  if (!class_weights_dict) {
    return {};
  }

  std::map</*class_name*/ std::string, /*weights*/ VectorData> class_weights;
  for (const std::string& class_name : classes) {
    const auto* const list = class_weights_dict->FindList(class_name);
    if (!list) {
      return {};
    }

    std::vector<float> class_coef_weights;
    class_coef_weights.reserve(list->size());
    for (const base::Value& item : *list) {
      if (!item.is_double() && !item.is_int()) {
        return {};
      }

      class_coef_weights.push_back(static_cast<float>(item.GetDouble()));
    }

    class_weights[class_name] = VectorData(std::move(class_coef_weights));
  }
  return class_weights;
}

std::map<std::string, double> ParsePipelineClassifierBiases(
    const base::Value::Dict& dict,
    std::vector<std::string> classes) {
  const auto* const biases_list = dict.FindList("biases");
  if (!biases_list || biases_list->size() != classes.size()) {
    return {};
  }

  std::map</*class_name*/ std::string, /*bias*/ double> biases;
  for (size_t i = 0; i < biases_list->size(); i++) {
    const base::Value& bias = (*biases_list)[i];
    if (!bias.is_double() && !bias.is_int()) {
      return {};
    }

    biases[classes[i]] = bias.GetDouble();
  }
  return biases;
}

std::vector<std::string> ParsePipelineClassifierPostMatrixFunctions(
    const base::Value::Dict& dict) {
  const auto* const post_matrix_functions_list =
      dict.FindList("neural_post_matrix_functions");
  if (!post_matrix_functions_list) {
    return {};
  }

  std::vector<std::string> post_matrix_functions;
  post_matrix_functions.reserve(post_matrix_functions_list->size());
  for (const base::Value& item : *post_matrix_functions_list) {
    if (!item.is_string()) {
      return {};
    }

    const std::string& function_type = item.GetString();
    if (function_type.empty()) {
      return {};
    }

    post_matrix_functions.push_back(function_type);
  }
  return post_matrix_functions;
}

std::vector<std::vector<VectorData>> ParsePipelineClassifierMatrixData(
    const base::Value::Dict& dict) {
  const auto* const neural_matricies_names_list =
      dict.FindList("neural_matricies_names");

  if (!neural_matricies_names_list) {
    return {};
  }

  const auto* const neural_matricies_dimensions_dict =
      dict.FindDict("neural_matricies_dimensions");
  if (!neural_matricies_dimensions_dict) {
    return {};
  }

  const auto* const neural_matricies_data_dict =
      dict.FindDict("neural_matricies_data");
  if (!neural_matricies_data_dict) {
    return {};
  }

  std::vector<std::vector<VectorData>> matricies;
  for (const auto& matrix_name_value : *neural_matricies_names_list) {
    if (!matrix_name_value.is_string()) {
      return {};
    }

    const std::string& matrix_name = matrix_name_value.GetString();
    const auto* const matrix_dimensions =
        neural_matricies_dimensions_dict->FindList(matrix_name);
    if (!matrix_dimensions) {
      return {};
    }

    const base::Value& matrix_dimension = (*matrix_dimensions)[0];
    if (!matrix_dimension.is_double() && !matrix_dimension.is_int()) {
      return {};
    }

    std::vector<VectorData> matrix;
    size_t matrix_dimension_rows = matrix_dimension.GetInt();
    for (size_t j = 0; j < matrix_dimension_rows; j++) {
      const std::string matrix_row_name =
          base::StrCat({matrix_name, "-", std::to_string(j)});

      const auto* const matrix_row =
          neural_matricies_data_dict->FindList(matrix_row_name);
      if (!matrix_row) {
        return {};
      }

      std::vector<float> matrix_row_data;
      matrix_row_data.reserve(matrix_row->size());
      for (const base::Value& item : *matrix_row) {
        if (!item.is_double() && !item.is_int()) {
          return {};
        }

        matrix_row_data.push_back(static_cast<float>(item.GetDouble()));
      }

      matrix.emplace_back(std::move(matrix_row_data));
    }

    matricies.push_back(matrix);
  }
  return matricies;
}

absl::optional<LinearModel> ParsePipelineClassifierLinear(
    const base::Value::Dict& dict) {
  const std::string* const classifier_type = dict.FindString("classifier_type");
  if (!classifier_type || *classifier_type != "LINEAR") {
    return absl::nullopt;
  }
  std::vector<std::string> classes = ParsePipelineClassifierClasses(dict);
  if (classes.empty()) {
    return absl::nullopt;
  }

  std::map<std::string, VectorData> class_weights =
      ParsePipelineClassifierWeights(dict, classes);
  if (class_weights.empty()) {
    return absl::nullopt;
  }

  std::map<std::string, double> biases =
      ParsePipelineClassifierBiases(dict, classes);
  if (biases.empty()) {
    return absl::nullopt;
  }

  return LinearModel(std::move(class_weights), std::move(biases));
}

absl::optional<NeuralModel> ParsePipelineClassifierNeural(
    const base::Value::Dict& dict) {
  const std::string* const classifier_type = dict.FindString("classifier_type");
  if (!classifier_type || *classifier_type != "NEURAL") {
    return absl::nullopt;
  }

  std::vector<std::string> classes = ParsePipelineClassifierClasses(dict);
  if (classes.empty()) {
    return absl::nullopt;
  }

  std::vector<std::string> post_matrix_functions =
      ParsePipelineClassifierPostMatrixFunctions(dict);

  std::vector<std::vector<VectorData>> matricies =
      ParsePipelineClassifierMatrixData(dict);

  return NeuralModel(std::move(matricies), std::move(post_matrix_functions),
                     std::move(classes));
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
      ParsePipelineClassifierLinear(*classifier_dict);

  absl::optional<NeuralModel> neural_model =
      ParsePipelineClassifierNeural(*classifier_dict);

  if ((!linear_model) && (!neural_model)) {
    return absl::nullopt;
  }
  if (!linear_model) {
    linear_model = LinearModel();
  }
  if (!neural_model) {
    neural_model = NeuralModel();
  }

  return PipelineInfo(*version, *timestamp, *locale,
                      std::move(*transformations), std::move(*linear_model),
                      std::move(*neural_model));
}

}  // namespace brave_ads::ml::pipeline
