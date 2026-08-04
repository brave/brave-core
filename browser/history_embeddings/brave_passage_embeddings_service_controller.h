// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/scoped_observation.h"
#include "base/types/optional_ref.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "components/optimization_guide/core/delivery/model_info.h"
#include "components/passage_embeddings/core/passage_embeddings_service_controller.h"

namespace passage_embeddings {

// Runs EmbeddingGemma through the native LiteRT embedder in the sandboxed
// passage embeddings utility process. A single instance is shared across
// profiles, accessed via Get(), to avoid loading the model more than once.
//
// Brave does not use the optimization guide tflite model that upstream's
// embedder relies on; the model is delivered by the local AI component updater
// (LocalModelsUpdaterState) and its file paths are resolved in
// OnLocalModelsReady.
class BravePassageEmbeddingsServiceController
    : public PassageEmbeddingsServiceController,
      public local_ai::LocalModelsUpdaterState::Observer {
 public:
  static BravePassageEmbeddingsServiceController* Get();

  BravePassageEmbeddingsServiceController(
      const BravePassageEmbeddingsServiceController&) = delete;
  BravePassageEmbeddingsServiceController& operator=(
      const BravePassageEmbeddingsServiceController&) = delete;

 private:
  friend class base::NoDestructor<BravePassageEmbeddingsServiceController>;

  BravePassageEmbeddingsServiceController();
  ~BravePassageEmbeddingsServiceController() override;

  // PassageEmbeddingsServiceController:
  // Swallow optimization_guide updates. Upstream's PassageEmbedderModelObserver
  // calls this whenever the tflite model component changes; we don't use that
  // model at all, so the notification is noise.
  bool MaybeUpdateModelInfo(
      base::optional_ref<const optimization_guide::ModelInfo> model_info)
      override;
  bool IsModelAvailable() override;
  EmbedderMetadata GetEmbedderMetadata() override;

  // local_ai::LocalModelsUpdaterState::Observer:
  void OnLocalModelsReady(const base::FilePath& install_dir) override;

  // Reply for the OnLocalModelsReady existence check: records the LiteRT model
  // paths and notifies observers only when both files are actually on disk.
  void OnLitertModelChecked(const base::FilePath& embeddings_model_path,
                            const base::FilePath& sp_model_path,
                            bool models_exist);

  base::ScopedObservation<local_ai::LocalModelsUpdaterState,
                          local_ai::LocalModelsUpdaterState::Observer>
      updater_state_observation_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
