// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/scoped_observation.h"
#include "base/types/optional_ref.h"
#include "brave/browser/history_embeddings/brave_passage_embeddings_service.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "components/optimization_guide/core/delivery/model_info.h"
#include "components/passage_embeddings/core/passage_embeddings_service_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace passage_embeddings {

// Runs the passage embeddings model in-process rather than in a sandboxed
// utility process, because Brave serves embeddings from its own local AI
// service.
//
// A single instance is shared across profiles, accessed via Get(), to avoid
// loading the model more than once.
//
// Brave does not use the optimization guide tflite model that upstream's
// embedder relies on, so the model metadata and download paths are overridden
// to no-ops.
class BravePassageEmbeddingsServiceController
    : public PassageEmbeddingsServiceController,
      public ProfileObserver,
      public local_ai::LocalModelsUpdaterState::Observer {
 public:
  static BravePassageEmbeddingsServiceController* Get();

  BravePassageEmbeddingsServiceController(
      const BravePassageEmbeddingsServiceController&) = delete;
  BravePassageEmbeddingsServiceController& operator=(
      const BravePassageEmbeddingsServiceController&) = delete;

  // Called by CreateBackgroundWebContents once the guest OTR profile is
  // available. Observes the profile so we can tear the service down
  // before it is destroyed on browser shutdown (otherwise the WebContents
  // inside the service would outlive its BrowserContext, tripping
  // BrowserContextImpl's `rph_with_bc_reference` NOTREACHED).
  void ObserveGuestOTRProfile(Profile* otr_profile);

  // Routes a renderer-side LocalAIService binding from
  // UntrustedLocalAIUI::BindInterface to the active embedder. No-op
  // when no service is alive.
  void BindLocalAIReceiver(
      mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver);

 private:
  friend class base::NoDestructor<BravePassageEmbeddingsServiceController>;

  BravePassageEmbeddingsServiceController();
  ~BravePassageEmbeddingsServiceController() override;

  // PassageEmbeddingsServiceController:
  // Swallow optimization_guide updates. Upstream's PassageEmbedderModelObserver
  // (created per-profile by PassageEmbedderModelObserverFactory) calls this
  // whenever the tflite model component changes; the base implementation
  // clears model paths and resets embedder_remote_ without touching service_,
  // which leaves the next GetEmbeddings to fail the new embedder against a
  // still-bound batch_embedder_ on the old service_. We don't use the
  // upstream model at all, so the notification is noise.
  bool MaybeUpdateModelInfo(
      base::optional_ref<const optimization_guide::ModelInfo> model_info)
      override;
  // These are not overrides, but they are named similarly to the ones in the
  // base class. The main difference is that these functions handle the service
  // in-process.
  void MaybeLaunchService();
  void ResetServiceRemote();

  bool IsModelAvailable() override;
  EmbedderMetadata GetEmbedderMetadata() override;
  void GetEmbeddings(std::vector<std::string> passages,
                     PassagePriority priority,
                     GetEmbeddingsResultCallback callback) override;

  // ProfileObserver:
  void OnProfileWillBeDestroyed(Profile* profile) override;

  // local_ai::LocalModelsUpdaterState::Observer:
  void OnLocalModelsReady(const base::FilePath& install_dir) override;

  // Posts the disk read for the five EmbeddingGemma files. Wired to
  // OnLocalModelFilesLoaded; the receiver waits on the mojo pipe until
  // BindPassageEmbedder is invoked there.
  void LoadModelFilesAndBind(
      mojo::PendingReceiver<mojom::PassageEmbedder> receiver);
  void OnLocalModelFilesLoaded(
      mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
      local_ai::mojom::ModelFilesPtr model_files);

  std::unique_ptr<BravePassageEmbeddingsService> service_;
  base::ScopedObservation<Profile, ProfileObserver> otr_profile_observation_{
      this};
  base::ScopedObservation<local_ai::LocalModelsUpdaterState,
                          local_ai::LocalModelsUpdaterState::Observer>
      updater_state_observation_{this};

  // Set true when LocalModelsUpdaterState reports the EmbeddingGemma
  // component is installed. Required for IsModelAvailable() to return
  // true.
  bool model_dir_ready_ = false;

  base::WeakPtrFactory<BravePassageEmbeddingsServiceController>
      weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
