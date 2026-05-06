// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_BATCH_PASSAGE_EMBEDDER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_BATCH_PASSAGE_EMBEDDER_H_

#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"

namespace passage_embeddings {

// In-process implementation of passage_embeddings::mojom::PassageEmbedder
// that wraps a renderer-side local_ai PassageEmbedder. Upstream's
// SchedulingEmbedder speaks this mojom; we translate its batch calls to
// the renderer's single-passage interface, processing passages
// sequentially so callbacks resolve with all embeddings in order.
class BraveBatchPassageEmbedder : public mojom::PassageEmbedder {
 public:
  BraveBatchPassageEmbedder(
      mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
      mojo::PendingRemote<local_ai::mojom::PassageEmbedder> renderer_embedder,
      base::OnceClosure on_disconnect);
  ~BraveBatchPassageEmbedder() override;

  BraveBatchPassageEmbedder(const BraveBatchPassageEmbedder&) = delete;
  BraveBatchPassageEmbedder& operator=(const BraveBatchPassageEmbedder&) =
      delete;

  // mojom::PassageEmbedder:
  void GenerateEmbeddings(const std::vector<std::string>& passages,
                          mojom::PassagePriority priority,
                          GenerateEmbeddingsCallback callback) override;

 private:
  struct Batch {
    Batch();
    ~Batch();
    Batch(Batch&&) noexcept;
    Batch& operator=(Batch&&) noexcept;

    std::vector<std::string> passages;
    std::vector<mojom::PassageEmbeddingsResultPtr> results;
    GenerateEmbeddingsCallback callback;
    size_t next_index = 0;
  };

  void ProcessNext();
  void OnOneEmbedding(const std::vector<double>& embedding);
  void FailCurrentBatch();
  void OnDisconnected();

  static std::optional<std::vector<float>> ToFloatEmbedding(
      const std::vector<double>& embedding);

  std::optional<Batch> current_batch_;
  std::deque<Batch> pending_batches_;

  mojo::Receiver<mojom::PassageEmbedder> receiver_;
  mojo::Remote<local_ai::mojom::PassageEmbedder> renderer_embedder_;
  base::OnceClosure on_disconnect_;

  base::WeakPtrFactory<BraveBatchPassageEmbedder> weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_BATCH_PASSAGE_EMBEDDER_H_
