// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_EMBEDDER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_EMBEDDER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/passage_embeddings/passage_embeddings_types.h"

class Profile;

namespace local_ai {
class CandleService;
}

namespace brave {

// BraveEmbedder implements the passage_embeddings::Embedder interface
// using Brave's EmbeddingGemma model via CandleService.
class BraveEmbedder : public passage_embeddings::Embedder {
 public:
  explicit BraveEmbedder(Profile* profile);
  ~BraveEmbedder() override;

  BraveEmbedder(const BraveEmbedder&) = delete;
  BraveEmbedder& operator=(const BraveEmbedder&) = delete;

  // passage_embeddings::Embedder:
  TaskId ComputePassagesEmbeddings(
      passage_embeddings::PassagePriority priority,
      std::vector<std::string> passages,
      ComputePassagesEmbeddingsCallback callback) override;
  void ReprioritizeTasks(passage_embeddings::PassagePriority priority,
                         const std::set<TaskId>& tasks) override;
  bool TryCancel(TaskId task_id) override;

 private:
  struct PendingTask {
    PendingTask();
    ~PendingTask();
    PendingTask(PendingTask&&);
    PendingTask& operator=(PendingTask&&);

    passage_embeddings::PassagePriority priority;
    std::vector<std::string> passages;
    ComputePassagesEmbeddingsCallback callback;
    size_t completed_count = 0;
    std::vector<passage_embeddings::Embedding> embeddings;
    bool cancelled = false;
  };

  void ProcessNextPassage(TaskId task_id);
  void OnEmbeddingResult(TaskId task_id,
                         size_t passage_index,
                         const std::vector<double>& embedding);

  TaskId next_task_id_ = 1;
  std::map<TaskId, PendingTask> pending_tasks_;

  raw_ptr<local_ai::CandleService> candle_service_;

  base::WeakPtrFactory<BraveEmbedder> weak_ptr_factory_{this};
};

}  // namespace brave

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_EMBEDDER_H_
