// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "components/optimization_guide/proto/models.pb.h"
#include "components/passage_embeddings/core/passage_embeddings_service_launcher.h"
#include "content/public/browser/service_process_host.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace passage_embeddings {

namespace {

// Launches the sandboxed Passage Embeddings utility process, whose LoadModels
// is chromium_src-overridden to run EmbeddingGemma on LiteRT's CompiledModel
// inside that process.
class LitertServiceLauncher : public PassageEmbeddingsServiceLauncher {
 public:
  static PassageEmbeddingsServiceLauncher& Create() {
    static base::NoDestructor<LitertServiceLauncher> launcher;
    return *launcher;
  }

  void LaunchService(mojo::PendingReceiver<mojom::PassageEmbeddingsService>
                         receiver) override {
    content::ServiceProcessHost::Launch<mojom::PassageEmbeddingsService>(
        std::move(receiver), content::ServiceProcessHost::Options()
                                 .WithDisplayName("Passage Embeddings Service")
                                 .Pass());
  }
  void OnServiceDisconnected(bool is_idle) override {}
  bool AllowedToLaunch() const override { return true; }
};

}  // namespace

// static
BravePassageEmbeddingsServiceController*
BravePassageEmbeddingsServiceController::Get() {
  static base::NoDestructor<BravePassageEmbeddingsServiceController> instance;
  return instance.get();
}

BravePassageEmbeddingsServiceController::
    BravePassageEmbeddingsServiceController()
    : PassageEmbeddingsServiceController(LitertServiceLauncher::Create()) {
  // AddObserver re-fires OnLocalModelsReady synchronously if the component is
  // already installed, so this also covers a model installed before startup.
  updater_state_observation_.Observe(
      local_ai::LocalModelsUpdaterState::GetInstance());
}

BravePassageEmbeddingsServiceController::
    ~BravePassageEmbeddingsServiceController() = default;

bool BravePassageEmbeddingsServiceController::MaybeUpdateModelInfo(
    base::optional_ref<const optimization_guide::ModelInfo> model_info) {
  // No-op: we don't consume optimization_guide's tflite model. See header.
  return false;
}

void BravePassageEmbeddingsServiceController::OnLocalModelsReady(
    const base::FilePath& install_dir) {
  // Drop a load still in flight for the dir this one replaces.
  weak_ptr_factory_.InvalidateWeakPtrs();
  // The component ships the LiteRT model in optimization guide's own layout:
  // model.tflite, model-info.pb, and the SentencePiece model listed in its
  // additional_files. Loading it here reads the version and the embedder
  // metadata from the component rather than hard-coding them, and reports no
  // model at all when an older component (or a withheld download) leaves any
  // of those files missing.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(
          &optimization_guide::LoadAndVerifyModelInfoOffThread,
          optimization_guide::proto::OPTIMIZATION_TARGET_PASSAGE_EMBEDDER,
          local_ai::LocalModelsUpdaterState::GetInstance()
              ->GetEmbeddingGemmaLitertDir()),
      base::BindOnce(
          &BravePassageEmbeddingsServiceController::OnLitertModelInfoLoaded,
          weak_ptr_factory_.GetWeakPtr()));
}

void BravePassageEmbeddingsServiceController::OnLocalModelsUnavailable() {
  // The files go with the component, so drop the model built from them.
  // Cancelling first stops a load in flight republishing what it just read.
  weak_ptr_factory_.InvalidateWeakPtrs();
  PassageEmbeddingsServiceController::MaybeUpdateModelInfo(std::nullopt);
}

void BravePassageEmbeddingsServiceController::OnLitertModelInfoLoaded(
    std::optional<optimization_guide::ModelInfo> model_info) {
  if (!model_info) {
    VLOG(1) << "No usable LiteRT model in the EmbeddingGemma component; "
               "passage embeddings disabled until it ships one";
  }
  // Upstream validates the metadata, records the model paths and notifies
  // observers. With no model info it clears the model recorded before, whose
  // dir the component updater removes once this version is installed.
  PassageEmbeddingsServiceController::MaybeUpdateModelInfo(model_info);
}

}  // namespace passage_embeddings
