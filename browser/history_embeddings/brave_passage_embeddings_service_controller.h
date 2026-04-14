// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/history_embeddings/brave_embedder.h"
#include "components/passage_embeddings/core/passage_embeddings_service_controller.h"

class Profile;

namespace passage_embeddings {

// Brave's implementation of PassageEmbeddingsServiceController.
// Instead of launching a separate service process, we use an in-process
// implementation that forwards to a single shared BraveEmbedder (which
// uses LocalAIService). The LocalAIService runs its WASM renderer on
// the guest OTR profile, so there is no per-profile state here.
class BravePassageEmbeddingsServiceController
    : public PassageEmbeddingsServiceController,
      public BraveEmbedder::Observer {
 public:
  static BravePassageEmbeddingsServiceController* Get();

  BravePassageEmbeddingsServiceController(
      const BravePassageEmbeddingsServiceController&) = delete;
  BravePassageEmbeddingsServiceController& operator=(
      const BravePassageEmbeddingsServiceController&) = delete;

  // Return the shared BraveEmbedder. Creates it lazily on first call
  // using |profile| to obtain the LocalAIService. The profile param
  // is needed by the chromium_src override that replaces
  // GetEmbedder() with GetBraveEmbedder(profile).
  Embedder* GetBraveEmbedder(Profile* profile);

 private:
  friend class base::NoDestructor<BravePassageEmbeddingsServiceController>;

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

  // Single shared embedder, created lazily on first GetBraveEmbedder()
  // call.
  std::unique_ptr<BraveEmbedder> embedder_;
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
