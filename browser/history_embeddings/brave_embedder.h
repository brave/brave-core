// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_EMBEDDER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_EMBEDDER_H_

#include <deque>
#include <set>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "mojo/public/cpp/bindings/remote.h"

class Profile;

namespace passage_embeddings {

// BraveEmbedder implements the Embedder interface using Brave's
// EmbeddingGemma model via LocalAIService.
//
// Processes one passage at a time sequentially. Between passages, the
// job queue is re-sorted by priority so that high-priority search
// queries can preempt queued indexing work.
class BraveEmbedder : public Embedder {
 public:
  explicit BraveEmbedder(Profile* profile);
  // Test-only: inject a pre-bound PassageEmbedder remote directly.
  explicit BraveEmbedder(
      mojo::PendingRemote<local_ai::mojom::PassageEmbedder> remote);
  ~BraveEmbedder() override;

  BraveEmbedder(const BraveEmbedder&) = delete;
  BraveEmbedder& operator=(const BraveEmbedder&) = delete;

  // passage_embeddings::Embedder:
  TaskId ComputePassagesEmbeddings(
      PassagePriority priority,
      std::vector<std::string> passages,
      ComputePassagesEmbeddingsCallback callback) override;
  void ReprioritizeTasks(PassagePriority priority,
                         const std::set<TaskId>& tasks) override;
  bool TryCancel(TaskId task_id) override;

 private:
  struct Job {
    Job();
    ~Job();
    Job(Job&&) noexcept;
    Job& operator=(Job&&) noexcept;

    PassagePriority priority = PassagePriority::kPassive;
    TaskId task_id = 0;
    std::vector<std::string> passages;
    ComputePassagesEmbeddingsCallback callback;
    // Partial results; embeddings.size() == number of passages completed.
    std::vector<Embedding> embeddings;
  };

  void AcquirePassageEmbedder();
  void OnPassageEmbedderAcquired(
      mojo::PendingRemote<local_ai::mojom::PassageEmbedder> remote);
  void OnPassageEmbedderDisconnected();
  void OnIdleTimeout();
  void RestartIdleTimer();
  void FailAllPendingTasks();
  void ProcessNextPassage();
  void OnSinglePassageComplete(const std::vector<double>& embedding);

  bool acquiring_embedder_ = false;
  bool work_submitted_ = false;
  TaskId next_task_id_ = 1;
  std::deque<Job> jobs_;

  mojo::Remote<local_ai::mojom::LocalAIService> local_ai_service_;
  mojo::Remote<local_ai::mojom::PassageEmbedder> passage_embedder_;

  // Mojo's set_idle_handler() relies on the receiver sending MessageAck and
  // NotifyIdle control messages, but the JS Mojo bindings don't implement
  // this protocol. Use an explicit timer instead: it starts when all tasks
  // complete and resets when new work arrives.
  base::OneShotTimer idle_timer_;

  base::WeakPtrFactory<BraveEmbedder> weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_EMBEDDER_H_
