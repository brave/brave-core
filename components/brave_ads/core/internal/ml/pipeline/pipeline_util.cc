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

#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/distribution_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation_util.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/normalization_transformation.h"

namespace brave_ads::ml::pipeline {

namespace {

constexpr char kBiasesKey[] = "biases";
constexpr char kCategoriesVectorDimensionsKey[] = "dimension";
constexpr char kClassesKey[] = "classes";

constexpr char kClassifierKey[] = "classifier";
constexpr char kClassifierTypeKey[] = "classifier_type";
constexpr char kClassifierTypeLinearKey[] = "LINEAR";
constexpr char kClassifierTypeNeuralNetworkKey[] = "NEURAL";

constexpr char kClassWeightsKey[] = "class_weights";
constexpr char kHuffmanCodingKey[] = "huffman_coding";
constexpr char kLocaleKey[] = "locale";
constexpr char kNumberBucketsKey[] = "num_buckets";

constexpr char kNeuralNetworkMatrixNamesKey[] = "neural_matricies_names";
constexpr char kNeuralNetworkMatrixDimensionsKey[] =
    "neural_matricies_dimensions";
constexpr char kNeuralNetworkPostMatrixFunctionsKey[] =
    "neural_post_matrix_functions";
constexpr char kNeuralNetworkMatrixDataKey[] = "neural_matricies_data";

constexpr char kNgramsRangeKey[] = "ngrams_range";
constexpr char kParamsKey[] = "params";
constexpr char kTimestampKey[] = "timestamp";
constexpr char kTokenCategoriesMappingKey[] = "mapping";

constexpr char kTransformationsKey[] = "transformations";
constexpr char kTransformationTypeKey[] = "transformation_type";
constexpr char kTransformationTypeHashedNgramsKey[] = "HASHED_NGRAMS";
constexpr char kTransformationTypeMappedTokensKey[] = "MAPPED_TOKENS";
constexpr char kTransformationTypeNormalizeKey[] = "NORMALIZE";
constexpr char kTransformationTypeToDistributionKey[] = "TO_DISTRIBUTION";
constexpr char kTransformationTypeToLowerKey[] = "TO_LOWER";

constexpr char kVersionKey[] = "version";

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

std::map<std::string, std::vector<int>> FillHuffmanCodingMapping(
    const base::Value::Dict* huffman_coding) {
  std::map<std::string, std::vector<int>> huffman_coding_mapping;
  for (const auto [character, coding] : *huffman_coding) {
    const auto* coding_bits = coding.GetIfList();
    if (!coding_bits) {
      continue;
    }

    std::string character_key = character;

    std::vector<int> mapped_coding_bits;
    mapped_coding_bits.reserve(coding_bits->size());
    for (const base::Value& coding_bit : *coding_bits) {
      if (!coding_bit.is_double() && !coding_bit.is_int()) {
        return {};
      }
      mapped_coding_bits.push_back(static_cast<int>(coding_bit.GetInt()));
    }
    huffman_coding_mapping[character_key] = mapped_coding_bits;
  }
  return huffman_coding_mapping;
}

std::map<std::basic_string<unsigned char>, std::vector<unsigned char>>
FillTokensCategoriesMapping(
    const base::Value::Dict* mapping,
    const std::map<std::string, std::vector<int>>& huffman_coding_mapping) {
  std::map<std::basic_string<unsigned char>, std::vector<unsigned char>>
      token_categories_mapping;
  for (const auto [token, categories] : *mapping) {
    const auto* category_indexes = categories.GetIfList();
    if (!category_indexes) {
      continue;
    }

    std::string token_text = token;
    absl::optional<std::basic_string<unsigned char>> compressed_token =
        CompressToken(token_text, huffman_coding_mapping);
    if (!compressed_token) {
      continue;
    }

    std::vector<unsigned char> mapped_category_indexes;
    mapped_category_indexes.reserve(category_indexes->size());
    for (const base::Value& category_index : *category_indexes) {
      if (!category_index.is_double() && !category_index.is_int()) {
        return {};
      }
      mapped_category_indexes.push_back(
          static_cast<unsigned char>(category_index.GetInt()));
    }
    token_categories_mapping[*compressed_token] = mapped_category_indexes;
  }
  return token_categories_mapping;
}

absl::optional<TransformationPtr> ParsePipelineTransformationMappedTokens(
    const auto* const transformation_dict) {
  const absl::optional<int> dimension =
      transformation_dict->FindInt(kCategoriesVectorDimensionsKey);
  if (!dimension) {
    return absl::nullopt;
  }

  const auto* const huffman_coding =
      transformation_dict->FindDict(kHuffmanCodingKey);
  if (!huffman_coding) {
    return absl::nullopt;
  }
  std::map<std::string, std::vector<int>> huffman_coding_mapping =
      FillHuffmanCodingMapping(huffman_coding);

  const auto* const mapping =
      transformation_dict->FindDict(kTokenCategoriesMappingKey);
  if (!mapping) {
    return absl::nullopt;
  }

  int vector_dimension = *dimension;
  std::map<std::basic_string<unsigned char>, std::vector<unsigned char>>
      token_categories_mapping =
          FillTokensCategoriesMapping(mapping, huffman_coding_mapping);
  if (token_categories_mapping.empty()) {
    return absl::nullopt;
  }

  return std::make_unique<MappedTokensTransformation>(
      vector_dimension, huffman_coding_mapping, token_categories_mapping);
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

  if (transformation_type == kTransformationTypeMappedTokensKey) {
    absl::optional<TransformationPtr> mapped_tokens_transformation =
        ParsePipelineTransformationMappedTokens(transformation_dict);
    if (!mapped_tokens_transformation) {
      return absl::nullopt;
    }
    return std::move(*mapped_tokens_transformation);
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

std::vector<std::string> FillPostMatrixFunctions(
    const base::Value::List* post_matrix_functions) {
  std::vector<std::string> filled_post_matrix_functions;
  filled_post_matrix_functions.reserve(post_matrix_functions->size());

  for (const base::Value& post_matrix_function : *post_matrix_functions) {
    if (!post_matrix_function.is_string()) {
      return {};
    }
    const std::string& post_matrix_function_type =
        post_matrix_function.GetString();
    if (post_matrix_function_type.empty()) {
      return {};
    }
    filled_post_matrix_functions.push_back(post_matrix_function_type);
  }
  return filled_post_matrix_functions;
}

std::vector<std::string> ParsePipelineClassifierPostMatrixFunctions(
    const base::Value::Dict& classifier) {
  const auto* const post_matrix_functions =
      classifier.FindList(kNeuralNetworkPostMatrixFunctionsKey);
  if (!post_matrix_functions) {
    return {};
  }

  std::vector<std::string> filled_post_matrix_functions =
      FillPostMatrixFunctions(post_matrix_functions);
  if (filled_post_matrix_functions.empty()) {
    return {};
  }
  return filled_post_matrix_functions;
}

std::vector<float> FillMatrixRow(const base::Value::List* matrix_row) {
  std::vector<float> matrix_row_data;
  matrix_row_data.reserve(matrix_row->size());
  for (const base::Value& weight : *matrix_row) {
    if (!weight.is_double() && !weight.is_int()) {
      return {};
    }
    matrix_row_data.push_back(static_cast<float>(weight.GetDouble()));
  }
  return matrix_row_data;
}

std::vector<VectorData> FillMatrix(
    const base::Value::Dict* neural_matricies_data,
    const std::string& matrix_name,
    size_t matrix_dimension_rows) {
  std::vector<VectorData> matrix;

  for (size_t j = 0; j < matrix_dimension_rows; j++) {
    const std::string matrix_row_name =
        base::StrCat({matrix_name, "-", std::to_string(j)});
    const auto* const matrix_row =
        neural_matricies_data->FindList(matrix_row_name);
    if (!matrix_row) {
      return {};
    }

    std::vector<float> matrix_row_data = FillMatrixRow(matrix_row);
    if (matrix_row_data.empty()) {
      return {};
    }
    matrix.emplace_back(std::move(matrix_row_data));
  }
  return matrix;
}

std::vector<std::vector<VectorData>> FillMatricies(
    const base::Value::List* neural_matricies_names,
    const base::Value::Dict* neural_matricies_dimensions,
    const base::Value::Dict* neural_matricies_data) {
  std::vector<std::vector<VectorData>> matricies;

  for (const auto& neural_matricies_name : *neural_matricies_names) {
    if (!neural_matricies_name.is_string()) {
      return {};
    }

    const std::string& matrix_name = neural_matricies_name.GetString();
    const auto* const matrix_dimensions =
        neural_matricies_dimensions->FindList(matrix_name);
    if (!matrix_dimensions) {
      return {};
    }
    const base::Value& matrix_dimension = (*matrix_dimensions)[0];
    if (!matrix_dimension.is_double() && !matrix_dimension.is_int()) {
      return {};
    }
    std::vector<VectorData> matrix = FillMatrix(
        neural_matricies_data, matrix_name, matrix_dimension.GetInt());
    if (matrix.empty()) {
      return {};
    }
    matricies.push_back(matrix);
  }
  return matricies;
}

std::vector<std::vector<VectorData>> ParsePipelineClassifierMatrixData(
    const base::Value::Dict& classifier) {
  const auto* const neural_matricies_names =
      classifier.FindList(kNeuralNetworkMatrixNamesKey);
  if (!neural_matricies_names) {
    return {};
  }

  const auto* const neural_matricies_dimensions =
      classifier.FindDict(kNeuralNetworkMatrixDimensionsKey);
  if (!neural_matricies_dimensions) {
    return {};
  }
  const auto* const neural_matricies_data =
      classifier.FindDict(kNeuralNetworkMatrixDataKey);
  if (!neural_matricies_data) {
    return {};
  }
  std::vector<std::vector<VectorData>> matricies =
      FillMatricies(neural_matricies_names, neural_matricies_dimensions,
                    neural_matricies_data);
  if (matricies.empty()) {
    return {};
  }
  return matricies;
}

absl::optional<NeuralModel> ParsePipelineClassifierNeural(
    const base::Value::Dict& classifier) {
  std::vector<std::string> classes = ParsePipelineClassifierClasses(classifier);
  if (classes.empty()) {
    return absl::nullopt;
  }
  std::vector<std::string> post_matrix_functions =
      ParsePipelineClassifierPostMatrixFunctions(classifier);
  std::vector<std::vector<VectorData>> matricies =
      ParsePipelineClassifierMatrixData(classifier);
  if (matricies.empty()) {
    return absl::nullopt;
  }
  if (matricies.size() != post_matrix_functions.size()) {
    return absl::nullopt;
  }
  if (matricies.back().size() != classes.size()) {
    return absl::nullopt;
  }

  return NeuralModel(std::move(matricies), post_matrix_functions,
                     std::move(classes));
}

int ParsePipelineValueVersion(base::Value::Dict& dict) {
  const absl::optional<int> version = dict.FindInt(kVersionKey);
  if (!version) {
    return 0;
  }
  return *version;
}

std::string ParsePipelineValueTimestamp(base::Value::Dict& dict) {
  const std::string* const timestamp = dict.FindString(kTimestampKey);
  if (!timestamp) {
    return {};
  }
  return *timestamp;
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

absl::optional<NeuralModel> ParsePipelineValueClassifierNeural(
    base::Value::Dict& dict,
    const std::string& classifier_type) {
  if (classifier_type != kClassifierTypeNeuralNetworkKey) {
    return absl::nullopt;
  }

  const auto* classifier = dict.FindDict(kClassifierKey);
  if (!classifier) {
    return absl::nullopt;
  }
  return ParsePipelineClassifierNeural(*classifier);
}

}  // namespace

absl::optional<PipelineInfo> ParsePipelineValue(base::Value::Dict dict) {
  const int version = ParsePipelineValueVersion(dict);
  const std::string timestamp = ParsePipelineValueTimestamp(dict);
  const std::string locale = ParsePipelineValueLocale(dict);
  if (version == 0 || timestamp.empty() || locale.empty()) {
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
  absl::optional<NeuralModel> neural_model =
      ParsePipelineValueClassifierNeural(dict, classifier_type);
  if (!linear_model && !neural_model) {
    return absl::nullopt;
  }

  return PipelineInfo(version, timestamp, locale, std::move(transformations),
                      std::move(linear_model), std::move(neural_model));
}

}  // namespace brave_ads::ml::pipeline
