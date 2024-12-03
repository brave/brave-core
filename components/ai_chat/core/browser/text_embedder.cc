/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/text_embedder.h"

#include <algorithm>
#include <cmath>
#include <ios>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <utility>

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
#include "cc/port/statusor.h"
#include "cc/task/text/text_embedder.h"
#include "tensorflow_lite_support/cc/task/core/proto/base_options.pb.h"
#include "tensorflow_lite_support/cc/task/core/proto/external_file.pb.h"
#include "tensorflow_lite_support/cc/task/text/proto/text_embedder_options.pb.h"
#include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/text/utils/text_op_resolver.h"

using TFLiteTextEmbedder = tflite::task::text::TextEmbedder;
using TFLiteTextEmbedderOptions = tflite::task::text::TextEmbedderOptions;
using tflite::task::text::CreateTextOpResolver;

namespace ai_chat {

size_t TextEmbedder::g_segment_size_ceiling_(300);
size_t TextEmbedder::g_segment_size_floor_(50);

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

void TextEmbedder::GetTopSimilarityWithPromptTilContextLimit(
    const std::string& prompt,
    const std::string& text,
    uint32_t context_limit,
    TopSimilarityCallback callback) {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
  if (text.empty() || prompt.empty()) {
    std::move(callback).Run(base::unexpected("Empty text or prompt."));
    return;
  }
  if (text.length() <= context_limit) {
    std::move(callback).Run(base::ok(text));
    return;
  }
  embedder_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &TextEmbedder::GetTopSimilarityWithPromptTilContextLimitInternal,
          weak_ptr_factory_.GetWeakPtr(), prompt, text, context_limit,
          base::BindPostTaskToCurrentDefault(std::move(callback))));
}

void TextEmbedder::CancelAllTasks() {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
  base::AutoLock auto_lock(lock_);
  // Calling from owner task runner intentionally so we can stop tflite
  // interpretor running on embedder task runner.
  if (tflite_text_embedder_) {
    tflite_text_embedder_->Cancel();
  }
}

void TextEmbedder::GetTopSimilarityWithPromptTilContextLimitInternal(
    const std::string& prompt,
    const std::string& text,
    uint32_t context_limit,
    TopSimilarityCallback callback) {
  DCHECK(embedder_task_runner_->RunsTasksInCurrentSequence());
  if (!tflite_text_embedder_) {
    std::move(callback).Run(
        base::unexpected("TextEmbedder is not initialized."));
    return;
  }
  auto text_hash = base::FastHash(base::as_byte_span(text));
  if (text_hash != text_hash_) {
    text_hash_ = text_hash;
    segments_ = SplitSegments(text);
    base::ElapsedTimer timer;
    auto status = EmbedSegments();
    if (!status.ok()) {
      std::move(callback).Run(base::unexpected(status.ToString()));
      return;
    }
    base::UmaHistogramMicrosecondsTimes(
        "Brave.AIChat.TextEmbedder.EmbedSegmentsInMicroseconds",
        timer.Elapsed());
  }
  if (segments_.size() != embeddings_.size()) {
    std::move(callback).Run(
        base::unexpected("Segments and embeddings size mismatch."));
    return;
  }

  std::vector<ScoreType> ranked_sentences;
  auto maybe_prompt_embed = tflite_text_embedder_->Embed(prompt);
  if (!maybe_prompt_embed.ok()) {
    std::move(callback).Run(
        base::unexpected(maybe_prompt_embed.status().ToString()));
    return;
  }
  for (size_t i = 0; i < embeddings_.size(); ++i) {
    auto maybe_similarity = tflite_text_embedder_->CosineSimilarity(
        maybe_prompt_embed->embeddings(0).feature_vector(),
        embeddings_[i].embeddings(0).feature_vector());
    if (!maybe_similarity.ok()) {
      std::move(callback).Run(
          base::unexpected(maybe_similarity.status().ToString()));
      return;
    }
    ranked_sentences.emplace_back(i, maybe_similarity.value());
  }
  auto maybe_refined_page_content =
      RefineTopKSimilarity(std::move(ranked_sentences), context_limit);
  if (!maybe_refined_page_content.has_value()) {
    std::move(callback).Run(
        base::unexpected(maybe_refined_page_content.error()));
    return;
  }
  VLOG(4) << "Refined page content: " << maybe_refined_page_content.value();
  std::move(callback).Run(base::ok(maybe_refined_page_content.value()));
}

std::vector<std::string> TextEmbedder::SplitSegments(const std::string& text) {
  DCHECK(embedder_task_runner_->RunsTasksInCurrentSequence());
  auto segments = base::SplitStringUsingSubstr(
      text, ". ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (segments.size() < g_segment_size_floor_) {
    segments = base::SplitString(text, " !?", base::TRIM_WHITESPACE,
                                 base::SPLIT_WANT_NONEMPTY);
  }
  DVLOG(4) << "Segments: " << segments.size();
  if (segments.size() > g_segment_size_ceiling_) {
    std::vector<std::string> new_segments;
    size_t join_size = static_cast<size_t>(
        std::ceil(segments.size() / g_segment_size_ceiling_));
    std::string new_segment = "";
    for (size_t i = 0; i < segments.size(); ++i) {
      base::StrAppend(&new_segment, {segments[i]});
      if ((i + 1) % join_size == 0) {
        new_segments.push_back(new_segment);
        new_segment = "";
      } else if (i != segments.size() - 1) {
        base::StrAppend(&new_segment, {" "});
      }
      if (i == segments.size() - 1 && !new_segment.empty()) {
        new_segments.push_back(new_segment);
        new_segment = "";
      }
    }
    segments = new_segments;
    DVLOG(4) << "New Segments: " << segments.size();
  }
  return segments;
}

base::expected<std::string, std::string> TextEmbedder::RefineTopKSimilarity(
    std::vector<ScoreType> ranked_sentences,
    uint32_t context_limit) {
  DCHECK(embedder_task_runner_->RunsTasksInCurrentSequence());

  if (segments_.size() != ranked_sentences.size()) {
    return base::unexpected("Segments and ranked sentences size mismatch.");
  }

  std::sort(ranked_sentences.begin(), ranked_sentences.end(),
            [](const ScoreType& lhs, const ScoreType& rhs) {
              return lhs.second > rhs.second;
            });
  std::vector<size_t> top_k_indices;
  size_t total_length = 0;
  std::string refined_page_content = "";
  for (const auto& ranked_sentence : ranked_sentences) {
    if (ranked_sentence.first >= segments_.size()) {
      return base::unexpected("Invalid ranked sentence index.");
    }
    const auto& segment = segments_[ranked_sentence.first];
    if (total_length + segment.size() > context_limit) {
      break;
    }
    total_length += segment.size();
    top_k_indices.push_back(ranked_sentence.first);
  }
  std::sort(top_k_indices.begin(), top_k_indices.end());
  for (size_t i = 0; i < top_k_indices.size(); ++i) {
    base::StrAppend(&refined_page_content, {segments_[top_k_indices[i]]});
    if (i != top_k_indices.size() - 1) {
      base::StrAppend(&refined_page_content, {". "});
    }
  }

  return base::ok(refined_page_content);
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

absl::Status TextEmbedder::EmbedSegments() {
  DCHECK(embedder_task_runner_->RunsTasksInCurrentSequence());
  if (segments_.empty()) {
    return absl::FailedPreconditionError("No segments to embed.");
  }
  embeddings_.clear();
  for (const auto& segment : segments_) {
    tflite::task::processor::EmbeddingResult embedding;
    auto status = EmbedText(segment, embedding);
    if (!status.ok()) {
      return status;
    }
    embeddings_.push_back(embedding);
  }
  return absl::OkStatus();
}

// static
void TextEmbedder::SetSegmentSizeCeilingForTesting(size_t ceiling) {
  CHECK_IS_TEST();
  g_segment_size_ceiling_ = ceiling;
}

// static
void TextEmbedder::SetSegmentSizeFloorForTesting(size_t floor) {
  CHECK_IS_TEST();
  g_segment_size_floor_ = floor;
}

}  // namespace ai_chat
