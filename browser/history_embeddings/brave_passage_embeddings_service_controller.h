// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "components/passage_embeddings/passage_embeddings_service_controller.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"

namespace brave {
class BraveEmbedder;
}

namespace passage_embeddings {

// Brave's implementation of PassageEmbeddingsServiceController.
// Instead of launching a separate service process, we use an in-process
// implementation that forwards to BraveEmbedder (which uses CandleService).
class BravePassageEmbeddingsServiceController
    : public PassageEmbeddingsServiceController {
 public:
  static BravePassageEmbeddingsServiceController* Get();

  BravePassageEmbeddingsServiceController(
      const BravePassageEmbeddingsServiceController&) = delete;
  BravePassageEmbeddingsServiceController& operator=(
      const BravePassageEmbeddingsServiceController&) = delete;

  // Return our BraveEmbedder directly for the factory to pass to HistoryEmbeddingsService
  Embedder* GetBraveEmbedder();

 private:
  friend class base::NoDestructor<BravePassageEmbeddingsServiceController>;

  // In-process implementation of PassageEmbedder that forwards to BraveEmbedder
  class BravePassageEmbedderImpl : public mojom::PassageEmbedder {
   public:
    explicit BravePassageEmbedderImpl(brave::BraveEmbedder* embedder);
    ~BravePassageEmbedderImpl() override;

    // mojom::PassageEmbedder:
    void GenerateEmbeddings(const std::vector<std::string>& passages,
                           mojom::PassagePriority priority,
                           GenerateEmbeddingsCallback callback) override;

   private:
    void OnEmbeddingsGenerated(
        GenerateEmbeddingsCallback callback,
        std::vector<std::string> passages,
        std::vector<Embedding> embeddings,
        Embedder::TaskId task_id,
        ComputeEmbeddingsStatus status);

    raw_ptr<brave::BraveEmbedder> embedder_;
    base::WeakPtrFactory<BravePassageEmbedderImpl> weak_ptr_factory_{this};
  };

  // In-process implementation of PassageEmbeddingsService
  class BravePassageEmbeddingsServiceImpl : public mojom::PassageEmbeddingsService {
   public:
    explicit BravePassageEmbeddingsServiceImpl(brave::BraveEmbedder* embedder);
    ~BravePassageEmbeddingsServiceImpl() override;

    // mojom::PassageEmbeddingsService:
    void LoadModels(mojom::PassageEmbeddingsLoadModelsParamsPtr model_params,
                   mojom::PassageEmbedderParamsPtr params,
                   mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
                   LoadModelsCallback callback) override;

   private:
    raw_ptr<brave::BraveEmbedder> embedder_;
    std::unique_ptr<BravePassageEmbedderImpl> embedder_impl_;
    std::unique_ptr<mojo::Receiver<mojom::PassageEmbedder>> embedder_receiver_;
  };

  BravePassageEmbeddingsServiceController();
  ~BravePassageEmbeddingsServiceController() override;

  // PassageEmbeddingsServiceController:
  void MaybeLaunchService() override;
  void ResetServiceRemote() override;
  bool EmbedderReady() override;
  EmbedderMetadata GetEmbedderMetadata() override;
  void GetEmbeddings(std::vector<std::string> passages,
                     PassagePriority priority,
                     GetEmbeddingsResultCallback callback) override;

  std::unique_ptr<brave::BraveEmbedder> brave_embedder_;
  std::unique_ptr<BravePassageEmbeddingsServiceImpl> service_impl_;
  std::unique_ptr<mojo::Receiver<mojom::PassageEmbeddingsService>> service_receiver_;

  base::WeakPtrFactory<BravePassageEmbeddingsServiceController>
      weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
