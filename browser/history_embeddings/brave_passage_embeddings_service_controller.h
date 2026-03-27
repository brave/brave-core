// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "brave/browser/history_embeddings/brave_embedder.h"
#include "brave/components/local_ai/core/local_ai.mojom.h"
#include "components/passage_embeddings/core/passage_embeddings_service_controller.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace passage_embeddings {

// Brave's implementation of PassageEmbeddingsServiceController.
// Uses a single shared embedder backed by the guest profile's OTR
// LocalAIService so the background WebContents is fully in-memory
// and not tied to any user profile.
//
// The guest profile is pre-created at startup via CreateProfileAsync
// and lives until browser shutdown — its kWaitingForFirstBrowserWindow
// keep-alive is never cleared because guest windows use OTR and don't
// trigger ClearFirstBrowserWindowKeepAlive.
class BravePassageEmbeddingsServiceController
    : public PassageEmbeddingsServiceController,
      public BraveEmbedder::Observer {
 public:
  static BravePassageEmbeddingsServiceController* Get();

  BravePassageEmbeddingsServiceController(
      const BravePassageEmbeddingsServiceController&) = delete;
  BravePassageEmbeddingsServiceController& operator=(
      const BravePassageEmbeddingsServiceController&) = delete;

  // Return the embedder proxy. Always non-null. Delegates to the real
  // BraveEmbedder once SetLocalAIServiceRemote has been called and the
  // embedder is lazily created. Before that, returns kModelUnavailable.
  Embedder* GetBraveEmbedder();

  // Set the LocalAIService remote for the guest profile. Called from
  // EnsureBrowserContextKeyedServiceFactoriesBuilt when the guest
  // profile is ready.
  void SetLocalAIServiceRemote(
      mojo::PendingRemote<local_ai::mojom::LocalAIService> remote);

 private:
  friend class base::NoDestructor<BravePassageEmbeddingsServiceController>;

  // Proxy that delegates to the real BraveEmbedder when available.
  // Returned by GetBraveEmbedder() so callers always get a non-null
  // Embedder* even before the guest profile is ready.
  class EmbedderProxy : public Embedder {
   public:
    EmbedderProxy();
    ~EmbedderProxy() override;

    void SetTarget(Embedder* target);

    // Embedder:
    TaskId ComputePassagesEmbeddings(
        PassagePriority priority,
        std::vector<std::string> passages,
        ComputePassagesEmbeddingsCallback callback) override;
    void ReprioritizeTasks(PassagePriority priority,
                           const std::set<TaskId>& tasks) override;
    bool TryCancel(TaskId task_id) override;

   private:
    raw_ptr<Embedder> target_ = nullptr;
    TaskId next_task_id_ = 1;
  };

  BravePassageEmbeddingsServiceController();
  ~BravePassageEmbeddingsServiceController() override;

  // BraveEmbedder::Observer:
  void OnEmbedderIdle() override;

  // PassageEmbeddingsServiceController:
  void MaybeLaunchService() override;
  void ResetServiceRemote() override;
  bool EmbedderReady() override;
  EmbedderMetadata GetEmbedderMetadata() override;
  void GetEmbeddings(std::vector<std::string> passages,
                     PassagePriority priority,
                     GetEmbeddingsResultCallback callback) override;

  EmbedderProxy embedder_proxy_;
  std::unique_ptr<BraveEmbedder> embedder_;
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
