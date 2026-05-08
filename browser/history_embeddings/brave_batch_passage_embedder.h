// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_BATCH_PASSAGE_EMBEDDER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_BATCH_PASSAGE_EMBEDDER_H_

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"

namespace passage_embeddings {

// In-process implementation of passage_embeddings::mojom::PassageEmbedder
// that owns the full renderer-side lifecycle for a single load:
// the background WebContents that hosts the WASM worker, the
// renderer-facing LocalAIService receiver set used by the WASM page
// to register its PassageEmbedderFactory, and the factory->Init +
// factory->Bind handshake.
//
// Construction kicks off background contents creation. Initialization
// proceeds once both halves are present: the renderer registers its
// PassageEmbedderFactory via RegisterPassageEmbedderFactory, and the
// owner (BravePassageEmbeddingsService) hands over loaded model files
// via SetModelFiles. The embedder then drives factory->Init and
// binds the per-embedder renderer pipe.
//
// Disconnect handling (renderer crash, factory drop, caller drop, or
// background contents teardown) routes through OnDisconnected, which
// fires on_disconnect so the owner can drop this instance and accept
// a new load.
//
// Upstream's SchedulingEmbedder (kept in the controller) speaks
// mojom::PassageEmbedder in batches; we translate to the renderer's
// single-passage interface, processing passages sequentially so
// callbacks resolve with all embeddings in order.
class BraveBatchPassageEmbedder
    : public mojom::PassageEmbedder,
      public local_ai::mojom::LocalAIService,
      public local_ai::BackgroundWebContents::Delegate {
 public:
  using BackgroundWebContentsCreatedCallback = base::OnceCallback<void(
      std::unique_ptr<local_ai::BackgroundWebContents>)>;
  using BackgroundWebContentsFactory = base::RepeatingCallback<void(
      local_ai::BackgroundWebContents::Delegate* delegate,
      BackgroundWebContentsCreatedCallback)>;

  BraveBatchPassageEmbedder(
      mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
      BackgroundWebContentsFactory background_web_contents_factory,
      base::OnceCallback<void(bool)> load_callback,
      base::OnceClosure on_disconnect);
  ~BraveBatchPassageEmbedder() override;

  BraveBatchPassageEmbedder(const BraveBatchPassageEmbedder&) = delete;
  BraveBatchPassageEmbedder& operator=(const BraveBatchPassageEmbedder&) =
      delete;

  // Hands the loaded model files in. Drives factory->Init if the
  // renderer factory is already registered; otherwise stashes the files
  // until RegisterPassageEmbedderFactory arrives.
  void SetModelFiles(local_ai::mojom::ModelFilesPtr model_files);

  // Adds a renderer-facing LocalAIService receiver. Installed as the
  // BindCallback in the static registry on BravePassageEmbeddingsService
  // when the controller creates the background WebContents.
  void BindLocalAIReceiver(
      mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver);

  base::WeakPtr<BraveBatchPassageEmbedder> GetWeakPtr();

  // mojom::PassageEmbedder:
  void GenerateEmbeddings(const std::vector<std::string>& passages,
                          mojom::PassagePriority priority,
                          GenerateEmbeddingsCallback callback) override;

  // local_ai::mojom::LocalAIService:
  void RegisterPassageEmbedderFactory(
      mojo::PendingRemote<local_ai::mojom::PassageEmbedderFactory> factory)
      override;

  // local_ai::BackgroundWebContents::Delegate:
  void OnBackgroundContentsDestroyed(
      local_ai::BackgroundWebContents::DestroyReason reason) override;

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

  void OnBackgroundContentsCreated(
      std::unique_ptr<local_ai::BackgroundWebContents> contents);
  void MaybeStartInit();
  void OnInitDone(bool success);
  void ProcessNext();
  void OnOneEmbedding(const std::vector<double>& embedding);
  void FailCurrentBatch();
  void OnDisconnected();

  static std::optional<std::vector<float>> ToFloatEmbedding(
      const std::vector<double>& embedding);

  std::unique_ptr<local_ai::BackgroundWebContents> background_web_contents_;
  BackgroundWebContentsFactory background_web_contents_factory_;
  mojo::ReceiverSet<local_ai::mojom::LocalAIService> local_ai_receivers_;

  std::optional<Batch> current_batch_;
  std::deque<Batch> pending_batches_;

  mojo::Receiver<mojom::PassageEmbedder> receiver_;
  mojo::Remote<local_ai::mojom::PassageEmbedderFactory> factory_;
  mojo::Remote<local_ai::mojom::PassageEmbedder> renderer_embedder_;

  // Stashed until both factory_ and pending_model_files_ are present;
  // moved into factory_->Init when the handshake starts.
  local_ai::mojom::ModelFilesPtr pending_model_files_;

  // Load sequence:
  //   kCreatingContents  -> ctor invoked the BackgroundWebContentsFactory
  //                         and is waiting for OnBackgroundContentsCreated.
  //   kAwaitingPrereqs   -> background contents up; waiting for the
  //                         renderer to RegisterPassageEmbedderFactory and
  //                         the owner to call SetModelFiles.
  //   kInitializing      -> both prereqs present; factory_->Init in flight.
  //   kReady             -> Init succeeded and the renderer-side
  //                         PassageEmbedder pipe is bound; batches flow.
  enum class LoadPhase {
    kCreatingContents,
    kAwaitingPrereqs,
    kInitializing,
    kReady,
  };
  LoadPhase phase_ = LoadPhase::kCreatingContents;

  // Run exactly once with true on successful Init+Bind or false on any
  // failure path (Init error, early disconnect before Init replies,
  // owner-side teardown before the load completes).
  base::OnceCallback<void(bool)> load_callback_;
  base::OnceClosure on_disconnect_;

  base::WeakPtrFactory<BraveBatchPassageEmbedder> weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_BATCH_PASSAGE_EMBEDDER_H_
