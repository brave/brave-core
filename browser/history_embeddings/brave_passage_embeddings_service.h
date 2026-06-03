// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/history_embeddings/brave_batch_passage_embedder.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"

namespace passage_embeddings {

// In-process implementation of
// passage_embeddings::mojom::PassageEmbeddingsService. Takes the place of
// the utility-process PassageEmbeddingsService that upstream Chrome
// launches — the PassageEmbeddingsServiceController binds its
// service_remote_ directly to an instance of this class, so no IPC hop
// is involved.
//
// The service is a thin factory: BravePassageEmbeddingsServiceController
// owns the file-loading lifecycle and hands a fully populated
// ModelFilesPtr to BindPassageEmbedder. The service constructs a
// BraveBatchPassageEmbedder around it; the embedder owns the full
// renderer-side lifecycle (background WebContents, LocalAIService
// receiver set, PassageEmbedderFactory remote, and the
// mojom::PassageEmbedder pipe back to the controller).
class BravePassageEmbeddingsService : public mojom::PassageEmbeddingsService {
 public:
  using BackgroundWebContentsFactory =
      BraveBatchPassageEmbedder::BackgroundWebContentsFactory;

  explicit BravePassageEmbeddingsService(
      BackgroundWebContentsFactory background_web_contents_factory);
  ~BravePassageEmbeddingsService() override;

  BravePassageEmbeddingsService(const BravePassageEmbeddingsService&) = delete;
  BravePassageEmbeddingsService& operator=(
      const BravePassageEmbeddingsService&) = delete;

  // Forwards a renderer-side LocalAIService binding request to the
  // active BatchEmbedder, if one exists. UntrustedLocalAIUI::BindInterface
  // reaches this through the controller singleton.
  void BindLocalAIReceiver(
      mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver);

  // Direct in-process equivalent of mojom::PassageEmbeddingsService::LoadModels
  // — constructs an embedder for the given receiver using the supplied
  // model files. The upstream mojom is shaped for a tflite +
  // sentencepiece embedder in a sandboxed utility process; we run
  // in-process with a five-file EmbeddingGemma model, so the struct
  // doesn't fit and the mojo hop adds no isolation. The controller
  // calls this directly and leaves service_remote_ unbound. See
  // README.md for details.
  void BindPassageEmbedder(
      mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
      local_ai::mojom::ModelFilesPtr model_files,
      base::OnceCallback<void(bool)> callback);

  // mojom::PassageEmbeddingsService:
  void LoadModels(mojom::PassageEmbeddingsLoadModelsParamsPtr model_params,
                  mojom::PassageEmbedderParamsPtr params,
                  mojo::PendingReceiver<mojom::PassageEmbedder> model,
                  LoadModelsCallback callback) override;

 private:
  void OnBatchEmbedderDisconnected();

  BackgroundWebContentsFactory background_web_contents_factory_;
  std::unique_ptr<BraveBatchPassageEmbedder> batch_embedder_;

  base::WeakPtrFactory<BravePassageEmbeddingsService> weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_
