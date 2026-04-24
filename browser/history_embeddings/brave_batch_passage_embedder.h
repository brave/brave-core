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
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"

namespace passage_embeddings {

// In-process implementation of passage_embeddings::mojom::PassageEmbedder
// that wraps a renderer-side local_ai PassageEmbedder.
//
// Owns the full renderer-side lifecycle for a single LoadModels call:
// it receives the PassageEmbedderFactory remote + loaded ModelFiles
// from BravePassageEmbeddingsService, drives factory->Init on
// construction, binds the per-embedder renderer pipe on success, and
// self-signals disconnect so the service can reset and accept a new
// load request. This mirrors upstream's PassageEmbedder, which
// similarly self-manages receiver, model load, and disconnect.
//
// Upstream's SchedulingEmbedder speaks mojom::PassageEmbedder in
// batches; we translate to the renderer's single-passage interface,
// processing passages sequentially so callbacks resolve with all
// embeddings in order.
class BraveBatchPassageEmbedder : public mojom::PassageEmbedder {
 public:
  BraveBatchPassageEmbedder(
      mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
      mojo::Remote<local_ai::mojom::PassageEmbedderFactory> factory,
      local_ai::mojom::ModelFilesPtr model_files,
      base::OnceCallback<void(bool)> load_callback,
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

  void OnInitDone(bool success);
  void ProcessNext();
  void OnOneEmbedding(const std::vector<double>& embedding);
  void FailCurrentBatch();
  void OnDisconnected();

  static std::optional<std::vector<float>> ToFloatEmbedding(
      const std::vector<double>& embedding);

  std::optional<Batch> current_batch_;
  std::deque<Batch> pending_batches_;

  mojo::Receiver<mojom::PassageEmbedder> receiver_;
  mojo::Remote<local_ai::mojom::PassageEmbedderFactory> factory_;
  mojo::Remote<local_ai::mojom::PassageEmbedder> renderer_embedder_;
  // Run exactly once with true on successful Init+Bind or false on any
  // failure path (Init error, early disconnect before Init replies).
  base::OnceCallback<void(bool)> load_callback_;
  base::OnceClosure on_disconnect_;

  base::WeakPtrFactory<BraveBatchPassageEmbedder> weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_BATCH_PASSAGE_EMBEDDER_H_
