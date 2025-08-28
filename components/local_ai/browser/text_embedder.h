/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_BROWSER_TEXT_EMBEDDER_H_
#define BRAVE_COMPONENTS_LOCAL_AI_BROWSER_TEXT_EMBEDDER_H_

#include <stddef.h>

#include <cstdint>
#include <map>
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
#include "components/tab_groups/tab_group_id.h"
#include "tensorflow_lite_support/cc/task/processor/proto/embedding.pb.h"
#include "third_party/abseil-cpp/absl/status/status.h"
#include "third_party/tflite_support/src/tensorflow_lite_support/cc/task/text/text_embedder.h"
#include "url/gurl.h"

namespace local_ai {
class YakeKeywordExtractor;
}

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

namespace local_ai {
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

  // Structure to represent a tab with separate title and URL
  struct TabInfo {
    std::u16string title;
    GURL url;
    std::string tab_content;
  };

  // Structure to represent a candidate tab with index and tab info
  struct CandidateTab {
    int index;
    TabInfo tab_info;
  };

  using SuggestTabsForGroupCallback =
      base::OnceCallback<void(absl::StatusOr<std::vector<int>>)>;
  // Suggest tabs to add to a group based on semantic similarity.
  // IMPORTANT: TextEmbedder must be initialized before calling this method.
  // Check IsInitialized() or call Initialize() first.
  void SuggestTabsForGroup(std::vector<TabInfo> group_tabs,
                           std::vector<CandidateTab> candidate_tabs,
                           SuggestTabsForGroupCallback callback);

  using SuggestGroupForTabCallback =
      base::OnceCallback<void(absl::StatusOr<tab_groups::TabGroupId>)>;
  // Suggest which existing group a tab should be added to.
  // IMPORTANT: TextEmbedder must be initialized before calling this method.
  // Check IsInitialized() or call Initialize() first.
  void SuggestGroupForTab(
      CandidateTab candidate_tab,
      std::map<tab_groups::TabGroupId, std::vector<TabInfo>> group_tabs,
      SuggestGroupForTabCallback callback);

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

  // Helper method to serialize TabInfo to string for embedding
  std::string SerializeTabInfo(const TabInfo& tab_info);

  // Helper method to extract keywords from text using YAKE algorithm
  std::string ExtractKeywords(const std::string& text, size_t max_keywords = 5);

  void SuggestTabsForGroupImpl(std::vector<TabInfo> group_tabs,
                               std::vector<CandidateTab> candidate_tabs,
                               SuggestTabsForGroupCallback callback);

  void SuggestGroupForTabImpl(
      CandidateTab candidate_tab,
      std::map<tab_groups::TabGroupId, std::vector<TabInfo>> group_tabs,
      SuggestGroupForTabCallback callback);

  std::vector<tflite::task::processor::EmbeddingResult> embeddings_;
  absl::Status EmbedText(const std::string& text,
                         tflite::task::processor::EmbeddingResult& embedding);

  std::vector<std::string> tabs_;
  absl::Status EmbedTabs();

  absl::StatusOr<tflite::task::processor::EmbeddingResult>
  CalculateTabGroupCentroid();

  static constexpr float COSINE_SIM_THRESHOLD = 0.75f;
  std::vector<int> getMostSimilarTabIndices(const std::vector<double>& vec,
                                            const std::vector<int>& id);

  // Lock used to ensure mutual exclusive access to |tflite_text_embedder_|
  // when setting unique_ptr and accessing it from |owner_task_runner_|.
  mutable base::Lock lock_;
  std::unique_ptr<tflite::task::text::TextEmbedder> tflite_text_embedder_;

  const base::FilePath model_path_;
  scoped_refptr<base::SequencedTaskRunner> owner_task_runner_;
  scoped_refptr<base::SequencedTaskRunner> embedder_task_runner_;

  std::unique_ptr<YakeKeywordExtractor> keyword_extractor_;

  base::WeakPtrFactory<TextEmbedder> weak_ptr_factory_{this};
};
}  // namespace local_ai
#endif  // BRAVE_COMPONENTS_LOCAL_AI_BROWSER_TEXT_EMBEDDER_H_
