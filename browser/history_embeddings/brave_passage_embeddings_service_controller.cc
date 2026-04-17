// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/local_ai/content/background_web_contents_impl.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "components/passage_embeddings/core/passage_embeddings_features.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"

namespace passage_embeddings {

namespace {

mojom::PassagePriority ToMojom(PassagePriority priority) {
  switch (priority) {
    case kUserInitiated:
      return mojom::PassagePriority::kUserInitiated;
    case kUrgent:
      return mojom::PassagePriority::kUrgent;
    case kPassive:
    case kLatent:
      return mojom::PassagePriority::kPassive;
  }
}

void InstallBindCallback(
    base::WeakPtr<BravePassageEmbeddingsService> weak_service,
    content::WebContents* web_contents) {
  auto bind_cb = base::BindRepeating(
      &BravePassageEmbeddingsService::BindLocalAIReceiver, weak_service);
  BravePassageEmbeddingsService::SetBindCallbackForWebContents(
      web_contents, std::move(bind_cb));
  task_manager::WebContentsTags::CreateForToolContents(
      web_contents, IDS_LOCAL_AI_TASK_MANAGER_TITLE);
}

void OnGuestProfileCreated(
    base::WeakPtr<BravePassageEmbeddingsService> weak_service,
    BravePassageEmbeddingsService::BackgroundWebContentsCreatedCallback
        callback,
    Profile* guest_profile) {
  CHECK(guest_profile);
  if (!weak_service) {
    return;
  }
  auto* otr = guest_profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  // Register the OTR profile with the controller so we tear the service
  // down before the profile is destroyed on shutdown — otherwise the
  // WebContents inside the service would outlive its BrowserContext.
  BravePassageEmbeddingsServiceController::Get()->ObserveGuestOTRProfile(otr);
  auto contents = std::make_unique<local_ai::BackgroundWebContentsImpl>(
      otr, GURL(local_ai::kUntrustedLocalAIURL), weak_service.get(),
      base::BindOnce(&InstallBindCallback, weak_service));
  std::move(callback).Run(std::move(contents));
}

void CreateBackgroundWebContents(
    local_ai::BackgroundWebContents::Delegate* delegate,
    BravePassageEmbeddingsService::BackgroundWebContentsCreatedCallback
        callback) {
  auto* service = static_cast<BravePassageEmbeddingsService*>(delegate);
  auto weak_service = service->GetWeakPtr();
  auto* profile_manager = g_browser_process->profile_manager();
  CHECK(profile_manager);
  profile_manager->CreateProfileAsync(
      ProfileManager::GetGuestProfilePath(),
      base::BindOnce(&OnGuestProfileCreated, std::move(weak_service),
                     std::move(callback)));
}

}  // namespace

// static
BravePassageEmbeddingsServiceController*
BravePassageEmbeddingsServiceController::Get() {
  static base::NoDestructor<BravePassageEmbeddingsServiceController> instance;
  return instance.get();
}

BravePassageEmbeddingsServiceController::
    BravePassageEmbeddingsServiceController() {
  // SchedulingEmbedder (owned by the base class) was added to
  // observer_list_ during base construction. It's waiting for a
  // metadata update before it dispatches any work. Our metadata is
  // static, so fire the notification now.
  observer_list_.Notify(&EmbedderMetadataObserver::EmbedderMetadataUpdated,
                        GetEmbedderMetadata());
}

BravePassageEmbeddingsServiceController::
    ~BravePassageEmbeddingsServiceController() = default;

bool BravePassageEmbeddingsServiceController::MaybeUpdateModelInfo(
    base::optional_ref<const optimization_guide::ModelInfo> model_info) {
  // No-op: we don't consume optimization_guide's tflite model. See header.
  return false;
}

void BravePassageEmbeddingsServiceController::MaybeLaunchService() {
  if (service_) {
    return;
  }
  service_ = std::make_unique<BravePassageEmbeddingsService>(
      base::BindRepeating(&CreateBackgroundWebContents),
      local_ai::LocalModelsUpdaterState::GetInstance());
  // service_remote_ is intentionally left unbound:
  // BravePassageEmbeddingsService exposes an in-process BindPassageEmbedder()
  // that we call directly from GetEmbeddings() instead of routing LoadModels
  // through a mojo pipe (the upstream mojom requires physical model files,
  // which we don't have).
}

void BravePassageEmbeddingsServiceController::ResetServiceRemote() {
  DVLOG(3) << "ResetServiceRemote (service_=" << (service_ ? "set" : "null")
           << ")";
  ResetEmbedderRemote();
  service_.reset();
  otr_profile_observation_.Reset();
}

void BravePassageEmbeddingsServiceController::ObserveGuestOTRProfile(
    Profile* otr_profile) {
  CHECK(otr_profile);
  if (otr_profile_observation_.IsObservingSource(otr_profile)) {
    return;
  }
  otr_profile_observation_.Reset();
  otr_profile_observation_.Observe(otr_profile);
}

void BravePassageEmbeddingsServiceController::OnProfileWillBeDestroyed(
    Profile* profile) {
  DVLOG(1) << "Guest OTR profile is being destroyed; tearing down service "
              "to release BackgroundWebContents";
  // ResetServiceRemote drops service_ (and with it the BackgroundWebContents)
  // and calls otr_profile_observation_.Reset() so we stop observing.
  ResetServiceRemote();
}

bool BravePassageEmbeddingsServiceController::EmbedderReady() {
  return true;
}

EmbedderMetadata
BravePassageEmbeddingsServiceController::GetEmbedderMetadata() {
  return EmbedderMetadata(/*model_version=*/1,
                          /*output_size=*/768,
                          /*search_score_threshold=*/0.45);
}

void BravePassageEmbeddingsServiceController::GetEmbeddings(
    std::vector<std::string> passages,
    PassagePriority priority,
    GetEmbeddingsResultCallback callback) {
  if (passages.empty()) {
    std::move(callback).Run({}, ComputeEmbeddingsStatus::kSuccess);
    return;
  }

  if (!embedder_remote_) {
    MaybeLaunchService();
    auto receiver = embedder_remote_.BindNewPipeAndPassReceiver();
    // When the embedder pipe goes idle or disconnects, tear the whole
    // service down to free the WASM renderer.
    embedder_remote_.set_disconnect_handler(base::BindOnce(
        &BravePassageEmbeddingsServiceController::ResetServiceRemote,
        base::Unretained(this)));
    embedder_remote_.set_idle_handler(
        kEmbedderTimeout.Get(),
        base::BindRepeating(
            &BravePassageEmbeddingsServiceController::ResetServiceRemote,
            base::Unretained(this)));
    DVLOG(3) << "GetEmbeddings: binding new PassageEmbedder via service_";
    service_->BindPassageEmbedder(
        std::move(receiver), base::BindOnce([](bool success) {
          DVLOG_IF(1, !success)
              << "BravePassageEmbeddingsService reported BindPassageEmbedder "
                 "failure; this batch will return an empty result";
        }));
  }

  embedder_remote_->GenerateEmbeddings(
      std::move(passages), ToMojom(priority),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(
              [](GetEmbeddingsResultCallback cb,
                 std::vector<mojom::PassageEmbeddingsResultPtr> results) {
                auto status = results.empty()
                                  ? ComputeEmbeddingsStatus::kExecutionFailure
                                  : ComputeEmbeddingsStatus::kSuccess;
                std::move(cb).Run(std::move(results), status);
              },
              std::move(callback)),
          std::vector<mojom::PassageEmbeddingsResultPtr>()));
}

}  // namespace passage_embeddings
