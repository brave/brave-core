// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

#include <optional>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
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
// embedder relies on. The model is delivered by the local AI component updater
// (LocalModelsUpdaterState) as a model dir laid out the way optimization guide
// expects, so OnLocalModelsReady loads its model-info.pb into a ModelInfo and
// feeds that to the base class.
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
  // Ignores optimization_guide updates. Upstream's PassageEmbedderModelObserver
  // calls this whenever the tflite model component changes; we don't use that
  // model at all, so the notification is noise. The component's own model info
  // reaches the base implementation from OnLitertModelInfoLoaded().
  bool MaybeUpdateModelInfo(
      base::optional_ref<const optimization_guide::ModelInfo> model_info)
      override;

  // local_ai::LocalModelsUpdaterState::Observer:
  void OnLocalModelsReady(const base::FilePath& install_dir) override;
  void OnLocalModelsUnavailable() override;

  // Reply for the model dir load posted by OnLocalModelsReady. `model_info` is
  // empty when the component ships no usable model.
  void OnLitertModelInfoLoaded(
      std::optional<optimization_guide::ModelInfo> model_info);

  base::ScopedObservation<local_ai::LocalModelsUpdaterState,
                          local_ai::LocalModelsUpdaterState::Observer>
      updater_state_observation_{this};

  // Guards the in-flight model dir load. Invalidated whenever the install dir
  // changes, so a load started for a dir that is no longer current cannot
  // publish the model it read.
  base::WeakPtrFactory<BravePassageEmbeddingsServiceController>
      weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
