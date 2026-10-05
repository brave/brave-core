// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_FAKE_EMBEDDER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_FAKE_EMBEDDER_H_

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"

namespace ai_chat {

// Embeds a passage by the animals it mentions: "cat", "dog" and "bird" each
// have a dimension, so passages about the same animal score about 1 against
// each other and about 0 against the rest. Embeds asynchronously, like the
// real embedder, and records what it embedded.
class FakeEmbedder : public passage_embeddings::Embedder {
 public:
  FakeEmbedder();
  FakeEmbedder(const FakeEmbedder&) = delete;
  FakeEmbedder& operator=(const FakeEmbedder&) = delete;
  ~FakeEmbedder() override;

  // The passages embedded so far, each with its priority, in order.
  const std::vector<
      std::pair<std::string, passage_embeddings::PassagePriority>>&
  embedded_passages() const {
    return embedded_passages_;
  }

  // Whether a passage containing `text` has been embedded.
  bool HasEmbedded(std::string_view text) const;

  // passage_embeddings::Embedder:
  Job ComputePassagesEmbeddings(
      passage_embeddings::PassagePriority priority,
      std::vector<std::string> passages,
      ComputePassagesEmbeddingsCallback callback) override;
  base::WeakPtr<Embedder> GetWeakPtr() override;

 private:
  struct PendingJob {
    PendingJob();
    PendingJob(PendingJob&&);
    PendingJob& operator=(PendingJob&&);
    ~PendingJob();

    passage_embeddings::PassagePriority priority =
        passage_embeddings::PassagePriority::kPassive;
    std::vector<std::string> passages;
    ComputePassagesEmbeddingsCallback callback;
  };

  // passage_embeddings::Embedder:
  void ReprioritizeJobs(passage_embeddings::PassagePriority priority,
                        const std::set<uint64_t>& job_ids) override;
  bool TryCancel(uint64_t job_id) override;

  void RunJob(uint64_t job_id);

  uint64_t next_job_id_ = 1;
  std::map<uint64_t, PendingJob> pending_jobs_;
  std::vector<std::pair<std::string, passage_embeddings::PassagePriority>>
      embedded_passages_;
  base::WeakPtrFactory<FakeEmbedder> weak_ptr_factory_{this};
};

// Provides valid metadata for a 4-dimensional model, and can change it.
class FakeEmbedderMetadataProvider
    : public passage_embeddings::EmbedderMetadataProvider {
 public:
  FakeEmbedderMetadataProvider();
  FakeEmbedderMetadataProvider(const FakeEmbedderMetadataProvider&) = delete;
  FakeEmbedderMetadataProvider& operator=(const FakeEmbedderMetadataProvider&) =
      delete;
  ~FakeEmbedderMetadataProvider() override;

  void SetMetadata(passage_embeddings::EmbedderMetadata metadata);

  // passage_embeddings::EmbedderMetadataProvider:
  void AddObserver(
      passage_embeddings::EmbedderMetadataObserver* observer) override;
  void RemoveObserver(
      passage_embeddings::EmbedderMetadataObserver* observer) override;

 private:
  passage_embeddings::EmbedderMetadata metadata_;
  base::ObserverList<passage_embeddings::EmbedderMetadataObserver> observers_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_FAKE_EMBEDDER_H_
