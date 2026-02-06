// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

#include <map>
#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "components/passage_embeddings/passage_embeddings_service_controller.h"

class Profile;

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

  // Return our BraveEmbedder for the given profile. Creates embedders lazily
  // per-profile. The controller is a singleton but embedders are per-profile.
  Embedder* GetBraveEmbedder(Profile* profile);

 private:
  friend class base::NoDestructor<BravePassageEmbeddingsServiceController>;

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

  // Per-profile embedders. The controller is a singleton but embedders are
  // created lazily per-profile.
  std::map<raw_ptr<Profile>, std::unique_ptr<brave::BraveEmbedder>>
      profile_embedders_;

  base::WeakPtrFactory<BravePassageEmbeddingsServiceController>
      weak_ptr_factory_{this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
