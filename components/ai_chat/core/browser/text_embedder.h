/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEXT_EMBEDDER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEXT_EMBEDDER_H_

#include <stddef.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/expected.h"
#include "tensorflow_lite_support/cc/task/processor/proto/embedding.pb.h"
#include "third_party/abseil-cpp/absl/status/status.h"
#include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/text/text_embedder.h"

namespace tflite {
namespace task {
namespace text {
class TextEmbedder;
}  // namespace text
}  // namespace task
}  // namespace tflite

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace ai_chat {
// TextEmbedder is a wrapper class for tflite::task::text::TextEmbedder.and
// runs all the operations on a separate sequence task runner to avoid blocking
// owner sequence runner ex. brwoser UI thread.
class TextEmbedder {
 public:
  static std::unique_ptr<TextEmbedder, base::OnTaskRunnerDeleter> Create(
      const base::FilePath& model_path,
      scoped_refptr<base::SequencedTaskRunner> embedder_task_runner);

  virtual ~TextEmbedder();
  TextEmbedder(const TextEmbedder&) = delete;
  TextEmbedder& operator=(const TextEmbedder&) = delete;

  virtual bool IsInitialized() const;

  // Initialize tflite::task::text::TextEmbedder with the model file.
  // Since tflite on Windows doesn't support file path loading so we read the
  // model file and pass it through file content.
  using InitializeCallback = base::OnceCallback<void(bool)>;
  virtual void Initialize(InitializeCallback callback);

  // Comparing the similarity between the prompt and the text and return the
  // most relevant text to the prompt with the cosine similarity score. The most
  // relavent text will respect the order in the original text. The returned
  // text will be filled until the context_limit.
  using TopSimilarityCallback =
      base::OnceCallback<void(base::expected<std::string, std::string>)>;
  virtual void GetTopSimilarityWithPromptTilContextLimit(
      const std::string& prompt,
      const std::string& text,
      uint32_t context_limit,
      TopSimilarityCallback callback);

  // Cancel all the pending tflite tasks on the embedder task runner.
  // Should be used right before the TextEmbedder is destroyed to avoid long
  // running tflite tasks blocking shutdown.
  void CancelAllTasks();

 protected:
  explicit TextEmbedder(
      const base::FilePath& model_path,
      scoped_refptr<base::SequencedTaskRunner> embedder_task_runner);

 private:
  friend class TextEmbedderUnitTest;

  void InitializeEmbedder(base::OnceCallback<void(bool)> callback);

  void GetTopSimilarityWithPromptTilContextLimitInternal(
      const std::string& prompt,
      const std::string& text,
      uint32_t context_limit,
      TopSimilarityCallback callback);

  // Split the text into segments with maximum segment size limit.
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

  static void SetSegmentSizeCeilingForTesting(size_t ceiling);
  static void SetSegmentSizeFloorForTesting(size_t floor);
  static size_t g_segment_size_floor_;
  static size_t g_segment_size_ceiling_;
  size_t text_hash_ = 0;
  std::vector<std::string> segments_;
  std::vector<tflite::task::processor::EmbeddingResult> embeddings_;

  // Lock used to ensure mutual exclusive access to |tflite_text_embedder_|
  // when setting unique_ptr and accessing it from |owner_task_runner_|.
  mutable base::Lock lock_;
  std::unique_ptr<tflite::task::text::TextEmbedder> tflite_text_embedder_;

  const base::FilePath model_path_;
  scoped_refptr<base::SequencedTaskRunner> owner_task_runner_;
  scoped_refptr<base::SequencedTaskRunner> embedder_task_runner_;

  base::WeakPtrFactory<TextEmbedder> weak_ptr_factory_{this};
};
}  // namespace ai_chat
#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEXT_EMBEDDER_H_
