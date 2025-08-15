/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/text_embedder.h"

#include <algorithm>
#include <cmath>
#include <ios>
#include <iostream>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/hash/hash.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions_internal_overloads.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/task/bind_post_task.h"
#include "base/timer/elapsed_timer.h"
#include "third_party/tflite_support/src/tensorflow_lite_support/cc/port/statusor.h"
#include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/core/proto/base_options.pb.h"
#include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/core/proto/external_file.pb.h"
#include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/text/proto/text_embedder_options.pb.h"
#include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/text/text_embedder.h"
#include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/text/utils/text_op_resolver.h"

using TFLiteTextEmbedder = tflite::task::text::TextEmbedder;
using TFLiteTextEmbedderOptions = tflite::task::text::TextEmbedderOptions;
using tflite::task::text::CreateTextOpResolver;

namespace ai_chat {

// static
std::unique_ptr<TextEmbedder, base::OnTaskRunnerDeleter> TextEmbedder::Create(
    const base::FilePath& model_path,
    scoped_refptr<base::SequencedTaskRunner> embedder_task_runner) {
  if (model_path.empty()) {
    return std::unique_ptr<TextEmbedder, base::OnTaskRunnerDeleter>(
        nullptr, base::OnTaskRunnerDeleter(embedder_task_runner));
  }
  auto embedder = std::unique_ptr<TextEmbedder, base::OnTaskRunnerDeleter>(
      new TextEmbedder(model_path, embedder_task_runner),
      base::OnTaskRunnerDeleter(embedder_task_runner));
  return embedder;
}

TextEmbedder::TextEmbedder(
    const base::FilePath& model_path,
    scoped_refptr<base::SequencedTaskRunner> embedder_task_runner)
    : model_path_(model_path),
      owner_task_runner_(base::SequencedTaskRunner::GetCurrentDefault()),
      embedder_task_runner_(embedder_task_runner) {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
}

TextEmbedder::~TextEmbedder() = default;

bool TextEmbedder::IsInitialized() const {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
  base::AutoLock auto_lock(lock_);
  return tflite_text_embedder_ != nullptr;
}

void TextEmbedder::Initialize(base::OnceCallback<void(bool)> callback) {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());

  embedder_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&TextEmbedder::InitializeEmbedder,
                     weak_ptr_factory_.GetWeakPtr(),
                     base::BindPostTaskToCurrentDefault(std::move(callback))));
}

void TextEmbedder::InitializeEmbedder(InitializeCallback callback) {
  DCHECK(embedder_task_runner_->RunsTasksInCurrentSequence());
  if (!base::PathExists(model_path_)) {
    std::move(callback).Run(false);
    return;
  }
  TFLiteTextEmbedderOptions options;
  std::string file_content;
  if (!base::ReadFileToString(model_path_, &file_content)) {
    std::move(callback).Run(false);
    return;
  }
  *options.mutable_base_options()
       ->mutable_model_file()
       ->mutable_file_content() = std::move(file_content);
  auto maybe_text_embedder =
      TFLiteTextEmbedder::CreateFromOptions(options, CreateTextOpResolver());
  if (!maybe_text_embedder.ok()) {
    VLOG(1) << maybe_text_embedder.status().ToString();
    std::move(callback).Run(false);
    return;
  }

  base::AutoLock auto_lock(lock_);
  tflite_text_embedder_ = std::move(maybe_text_embedder.value());
  std::move(callback).Run(true);
}

absl::Status TextEmbedder::EmbedText(
    const std::string& text,
    tflite::task::processor::EmbeddingResult& embedding) {
  DCHECK(embedder_task_runner_->RunsTasksInCurrentSequence());
  if (!tflite_text_embedder_) {
    return absl::FailedPreconditionError("TextEmbedder is not initialized.");
  }
  auto maybe_embedding = tflite_text_embedder_->Embed(text);
  if (!maybe_embedding.ok()) {
    return maybe_embedding.status();
  }
  embedding = maybe_embedding.value();
  return absl::OkStatus();
}

// Given an array of strings (tab-title + origin), generate embeddings for all
absl::Status TextEmbedder::EmbedTabs() {
  DCHECK(embedder_task_runner_->RunsTasksInCurrentSequence());
  if (tabs_.empty()) {
    return absl::FailedPreconditionError("No tabs to embed.");
  }
  embeddings_.clear();
  for (const auto& tab : tabs_) {
    tflite::task::processor::EmbeddingResult embedding;
    auto status = EmbedText(tab, embedding);
    if (!status.ok()) {
      return status;
    }
    embeddings_.push_back(embedding);
  }
  return absl::OkStatus();
}

// Given an array of tab embeddings, find their centroid, which is the mean of
// all embeddings (across each dimension)
absl::StatusOr<tflite::task::processor::EmbeddingResult>
TextEmbedder::CalculateTabGroupCentroid() {
  DCHECK(embedder_task_runner_->RunsTasksInCurrentSequence());

  // check if there are any embeddings
  if (embeddings_.empty()) {
    return absl::FailedPreconditionError("No tab embeddings to find centroid.");
  }

  // num of embeddings (tabs) in the group
  size_t num_embeddings = embeddings_.size();
  // dimensionality of an embedding
  size_t embed_size =
      embeddings_[0].embeddings(0).feature_vector().value_float().size();
  // create a dummy centroid and initialize it to all 0s
  tflite::task::processor::EmbeddingResult centroid;
  centroid = embeddings_[0];

  for (size_t i = 0; i < embed_size; ++i) {
    centroid.mutable_embeddings(0)->mutable_feature_vector()->set_value_float(
        i, 0.0f);
  }

  // Sum all the embeddings
  for (const auto& embedding : embeddings_) {
    for (size_t i = 0; i < embed_size; ++i) {
      centroid.mutable_embeddings(0)->mutable_feature_vector()->set_value_float(
          i, centroid.embeddings(0).feature_vector().value_float(i) +
                 embedding.embeddings(0).feature_vector().value_float(i));
    }
  }

  // Compute the average (centroid)
  for (size_t i = 0; i < embed_size; ++i) {
    centroid.mutable_embeddings(0)->mutable_feature_vector()->set_value_float(
        i, centroid.embeddings(0).feature_vector().value_float(i) /
               num_embeddings);
  }
  return centroid;
}

// Given 2 arrays, (1) containing strings (tab title + origin) for all tabs in a
// group and (2) containing strings of all open tabs (candidates), output
// indices for top 3 candidates.
absl::StatusOr<std::vector<size_t>> TextEmbedder::SuggestTabsForGroup(
    std::vector<std::pair<int, std::string>> group_tabs,
    std::vector<std::pair<int, std::string>> candidate_tabs) {

  // get embeddings for group tabs
  tabs_.clear();

  // extract only the strings
  std::vector<std::string> tab_titles;
  for (const auto& tab : group_tabs) {
    tab_titles.push_back(tab.second);
  }

  tabs_ = tab_titles;
  auto status_group = EmbedTabs();
  if (!status_group.ok()) {
    return absl::FailedPreconditionError(
        "Error generating embeddings for tabs in the group");
  }

  // get centroid of group tabs
  tflite::task::processor::EmbeddingResult group_centroid;
  auto group_centroid_output = CalculateTabGroupCentroid();
  if (!group_centroid_output.ok()) {
    return absl::FailedPreconditionError(
        "Error generating centroid for tab group");
  } else {
    group_centroid = group_centroid_output.value();
  }

  // get embeddings for candidate tabs
  tabs_.clear();
  tab_titles.clear();

  std::vector<std::int> tab_ids;
  
  for (const auto& tab : candidate_tabs) {
    tab_ids.push_back(tab.first);
    tab_titles.push_back(tab.second);
  }

  tabs_ = tab_titles;
  auto status = EmbedTabs();
  if (!status.ok()) {
    return absl::FailedPreconditionError(
        "Error generating embeddings for candidate tabs");
  }

  // save these embeddings
  std::vector<tflite::task::processor::EmbeddingResult> candidate_embeddings_;
  candidate_embeddings_ = embeddings_;

  std::vector<double> sim_scores;

  // get cosine simularity of each candiate tab with centroid
  for (size_t i = 0; i < candidate_embeddings_.size(); ++i) {
    auto maybe_similarity = tflite_text_embedder_->CosineSimilarity(
        candidate_embeddings_[i].embeddings(0).feature_vector(),
        group_centroid.embeddings(0).feature_vector());
    if (!maybe_similarity.ok()) {
      return absl::FailedPreconditionError(
          "Error calculating cosine similarity.");
    }
    sim_scores.push_back(maybe_similarity.value());
  }

  std::vector<size_t> top_indices;

  top_indices = getMostSimilarTabIndices(sim_scores, tab_ids);

  return top_indices;
}

std::vector<size_t> TextEmbedder::getMostSimilarTabIndices(
    const std::vector<double>& vec,
    const std::vector<std::int>& id) {
  // Create pairs of (value, index) for values above threshold
  std::vector<std::pair<double, size_t>> filtered_pairs;

  for (size_t i = 0; i < vec.size(); ++i) {
    if (vec[i] > COSINE_SIM_THRESHOLD) {
      filtered_pairs.emplace_back(vec[i], id[i]);
    }
  }

  // Sort by value in descending order
  std::sort(filtered_pairs.begin(), filtered_pairs.end(),
            [](const auto& a, const auto& b) {
              return a.first > b.first;  // Sort by value descending
            });

  // Extract sorted indices
  std::vector<size_t> indices_above_threshold;
  for (const auto& pair : filtered_pairs) {
    indices_above_threshold.push_back(pair.second);
  }

  return indices_above_threshold;
}

}  // namespace ai_chat
