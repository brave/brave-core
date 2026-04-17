// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
#define BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "base/scoped_observation.h"
#include "base/types/optional_ref.h"
#include "brave/browser/history_embeddings/brave_passage_embeddings_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_observer.h"
#include "components/optimization_guide/core/delivery/model_info.h"
#include "components/passage_embeddings/core/passage_embeddings_service_controller.h"

namespace passage_embeddings {

// Brave's subclass of PassageEmbeddingsServiceController. Mirrors
// ChromePassageEmbeddingsServiceController, except MaybeLaunchService()
// constructs an in-process BravePassageEmbeddingsService (bound to the
// base class's service_remote_) instead of launching a utility process.
//
// Upstream's SchedulingEmbedder, owned by the base class, drives the
// actual job queue; we just provide the transport.
class BravePassageEmbeddingsServiceController
    : public PassageEmbeddingsServiceController,
      public ProfileObserver {
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
  void MaybeLaunchService() override;
  void ResetServiceRemote() override;
  bool EmbedderReady() override;
  EmbedderMetadata GetEmbedderMetadata() override;
  void GetEmbeddings(std::vector<std::string> passages,
                     PassagePriority priority,
                     GetEmbeddingsResultCallback callback) override;

  // ProfileObserver:
  void OnProfileWillBeDestroyed(Profile* profile) override;

  std::unique_ptr<BravePassageEmbeddingsService> service_;
  base::ScopedObservation<Profile, ProfileObserver> otr_profile_observation_{
      this};
};

}  // namespace passage_embeddings

#endif  // BRAVE_BROWSER_HISTORY_EMBEDDINGS_BRAVE_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
