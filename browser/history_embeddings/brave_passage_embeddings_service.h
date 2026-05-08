// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/history_embeddings/brave_batch_passage_embedder.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"

namespace content {
class WebContents;
}

namespace passage_embeddings {

// In-process implementation of
// passage_embeddings::mojom::PassageEmbeddingsService. Takes the place of
// the utility-process PassageEmbeddingsService that upstream Chrome
// launches — the PassageEmbeddingsServiceController binds its
// service_remote_ directly to an instance of this class, so no IPC hop
// is involved.
//
// The service is intentionally thin: its only responsibility is reading
// the EmbeddingGemma model files from disk (driven by
// LocalModelsUpdaterState component-updater notifications) and feeding
// them to a BraveBatchPassageEmbedder, which owns the renderer-side
// lifecycle (background WebContents, LocalAIService receiver set,
// PassageEmbedderFactory remote, and the mojom::PassageEmbedder pipe
// to the controller).
//
// On BindPassageEmbedder we construct an embedder eagerly, kick off
// the file-load task on the ThreadPool, and hand the loaded files to
// the embedder once they arrive. Disconnect of the embedder
// (renderer crash, factory drop, caller drop, or background contents
// teardown) routes back through OnBatchEmbedderDisconnected so the
// service can drop the embedder and accept a new load.
//
// Also exposes the static WebContents -> BindCallback registry used by
// UntrustedLocalAIUI::BindInterface to route renderer-side
// LocalAIService bindings to the active embedder.
class BravePassageEmbeddingsService
    : public mojom::PassageEmbeddingsService,
      public local_ai::LocalModelsUpdaterState::Observer {
 public:
  using BackgroundWebContentsFactory =
      BraveBatchPassageEmbedder::BackgroundWebContentsFactory;

  BravePassageEmbeddingsService(
      BackgroundWebContentsFactory background_web_contents_factory,
      local_ai::LocalModelsUpdaterState* updater_state);
  ~BravePassageEmbeddingsService() override;

  BravePassageEmbeddingsService(const BravePassageEmbeddingsService&) = delete;
  BravePassageEmbeddingsService& operator=(
      const BravePassageEmbeddingsService&) = delete;

  // Registry for routing mojo binding requests from the background
  // WebContents (on guest OTR) back to the active embedder.
  // UntrustedLocalAIUI::BindInterface calls BindForWebContents; the
  // controller installs SetBindCallbackForWebContents when the
  // embedder creates its background contents.
  using BindCallback = base::RepeatingCallback<void(
      mojo::PendingReceiver<local_ai::mojom::LocalAIService>)>;
  static void SetBindCallbackForWebContents(content::WebContents* web_contents,
                                            BindCallback callback);
  static void RemoveBindCallbackForWebContents(
      content::WebContents* web_contents);
  static void BindForWebContents(
      content::WebContents* web_contents,
      mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver);

  // Direct in-process equivalent of mojom::PassageEmbeddingsService::LoadModels
  // — constructs a embedder for the given receiver and starts the
  // file-load task. The upstream mojom is shaped for a tflite +
  // sentencepiece embedder in a sandboxed utility process; we run
  // in-process with a five-file EmbeddingGemma model, so the struct
  // doesn't fit and the mojo hop adds no isolation. The controller
  // calls this directly and leaves service_remote_ unbound. See
  // README.md for details.
  void BindPassageEmbedder(
      mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
      base::OnceCallback<void(bool)> callback);

  // mojom::PassageEmbeddingsService:
  void LoadModels(mojom::PassageEmbeddingsLoadModelsParamsPtr model_params,
                  mojom::PassageEmbedderParamsPtr params,
                  mojo::PendingReceiver<mojom::PassageEmbedder> model,
                  LoadModelsCallback callback) override;

 private:
  // LocalModelsUpdaterState::Observer:
  void OnLocalModelsReady(const base::FilePath& install_dir) override;

  void MaybeLoadLocalModelFiles();
  void OnLocalModelFilesLoaded(local_ai::mojom::ModelFilesPtr model_files);
  void OnBatchEmbedderDisconnected();

  BackgroundWebContentsFactory background_web_contents_factory_;
  raw_ptr<local_ai::LocalModelsUpdaterState> updater_state_;

  std::unique_ptr<BraveBatchPassageEmbedder> batch_embedder_;
  // True between PostTask and OnLocalModelFilesLoaded.
  bool file_load_in_flight_ = false;

  base::WeakPtrFactory<BravePassageEmbeddingsService> weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_H_
