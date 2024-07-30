/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEXT_EMBEDDER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEXT_EMBEDDER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/text/text_embedder.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace ai_chat {
class TextEmbedder {
 public:
  static std::unique_ptr<TextEmbedder> Create(const base::FilePath& model_path);

  virtual ~TextEmbedder();
  TextEmbedder(const TextEmbedder&) = delete;
  TextEmbedder& operator=(const TextEmbedder&) = delete;

  using TopSimilarityCallback =
      base::OnceCallback<void(base::expected<std::string, std::string>)>;
  virtual void GetTopSimilarityWithPromptTilContextLimit(
      const std::string& prompt,
      const std::string& text,
      uint32_t context_limit,
      TopSimilarityCallback callback);

 protected:
  TextEmbedder();

 private:
  friend class TextEmbedderUnitTest;

  void GetTopSimilarityWithPromptTilContextLimitInternal(
      const std::string& prompt,
      const std::string& text,
      uint32_t context_limit,
      TopSimilarityCallback callback);

  std::vector<std::string> SplitSegments(const std::string& text);

  // (index, similarity)
  using ScoreType = std::pair<size_t, double>;
  // It will return the refined content respecting its original segements order.
  base::expected<std::string, std::string> RefineTopKSimilarity(
      std::vector<ScoreType> ranked_sentences,
      uint32_t context_limit);

  absl::Status EmbedText(const std::string& text,
                         tflite::task::processor::EmbeddingResult& embedding);

  absl::Status EmbedSegments();

  scoped_refptr<base::SequencedTaskRunner> GetEmbedderTaskRunner();

  static void SetSegmentSizeLimitForTesting(size_t limit);
  static size_t g_segment_size_limit_;
  size_t text_hash_ = 0;
  std::vector<std::string> segments_;
  std::vector<tflite::task::processor::EmbeddingResult> embeddings_;
  std::unique_ptr<tflite::task::text::TextEmbedder> tflite_text_embedder_;

  scoped_refptr<base::SequencedTaskRunner> owner_task_runner_;
  scoped_refptr<base::SequencedTaskRunner> embedder_task_runner_;

  std::unique_ptr<base::WeakPtrFactory<TextEmbedder>> weak_ptr_factory_;
};
}  // namespace ai_chat
#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEXT_EMBEDDER_H_
