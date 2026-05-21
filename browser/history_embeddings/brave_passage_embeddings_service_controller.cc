// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/browser/history_embeddings/brave_batch_passage_embedder.h"
#include "brave/components/local_ai/content/background_web_contents_impl.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "components/passage_embeddings/core/passage_embeddings_features.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "mojo/public/cpp/base/big_buffer.h"
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

void TagWebContentsForTaskManager(content::WebContents* web_contents) {
  task_manager::WebContentsTags::CreateForToolContents(
      web_contents, IDS_LOCAL_AI_TASK_MANAGER_TITLE);
}

void OnGuestProfileCreated(
    base::WeakPtr<BraveBatchPassageEmbedder> weak_embedder,
    BraveBatchPassageEmbedder::BackgroundWebContentsCreatedCallback callback,
    Profile* guest_profile) {
  CHECK(guest_profile);
  if (!weak_embedder) {
    return;
  }
  auto* otr = guest_profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  // Register the OTR profile with the controller so we tear the service
  // down before the profile is destroyed on shutdown — otherwise the
  // WebContents inside the embedder would outlive its BrowserContext.
  BravePassageEmbeddingsServiceController::Get()->ObserveGuestOTRProfile(otr);
  auto contents = std::make_unique<local_ai::BackgroundWebContentsImpl>(
      otr, GURL(local_ai::kUntrustedLocalAIURL), weak_embedder.get(),
      base::BindOnce(&TagWebContentsForTaskManager));
  std::move(callback).Run(std::move(contents));
}

void CreateBackgroundWebContents(
    local_ai::BackgroundWebContents::Delegate* delegate,
    BraveBatchPassageEmbedder::BackgroundWebContentsCreatedCallback callback) {
  auto* embedder = static_cast<BraveBatchPassageEmbedder*>(delegate);
  auto weak_embedder = embedder->GetWeakPtr();
  auto* profile_manager = g_browser_process->profile_manager();
  CHECK(profile_manager);
  profile_manager->CreateProfileAsync(
      ProfileManager::GetGuestProfilePath(),
      base::BindOnce(&OnGuestProfileCreated, std::move(weak_embedder),
                     std::move(callback)));
}

// Reads a file directly into BigBuffer storage. For files >64KB
// BigBuffer uses shared memory.
std::optional<mojo_base::BigBuffer> ReadFileToBigBuffer(
    const base::FilePath& path) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    DVLOG(0) << "Failed to open: " << path;
    return std::nullopt;
  }
  int64_t size = file.GetLength();
  if (size <= 0) {
    DVLOG(0) << "Empty or unreadable: " << path;
    return std::nullopt;
  }
  mojo_base::BigBuffer buffer(static_cast<size_t>(size));
  if (!file.ReadAndCheck(0, base::span<uint8_t>(buffer))) {
    DVLOG(0) << "Failed to read: " << path;
    return std::nullopt;
  }
  return buffer;
}

local_ai::mojom::ModelFilesPtr LoadLocalModelFilesFromDisk(
    const base::FilePath& weights_path,
    const base::FilePath& weights_dense1_path,
    const base::FilePath& weights_dense2_path,
    const base::FilePath& tokenizer_path,
    const base::FilePath& config_path) {
  auto weights = ReadFileToBigBuffer(weights_path);
  if (!weights) {
    return nullptr;
  }
  auto weights_dense1 = ReadFileToBigBuffer(weights_dense1_path);
  if (!weights_dense1) {
    return nullptr;
  }
  auto weights_dense2 = ReadFileToBigBuffer(weights_dense2_path);
  if (!weights_dense2) {
    return nullptr;
  }
  auto tokenizer = ReadFileToBigBuffer(tokenizer_path);
  if (!tokenizer) {
    return nullptr;
  }
  auto config = ReadFileToBigBuffer(config_path);
  if (!config) {
    return nullptr;
  }
  auto model_files = local_ai::mojom::ModelFiles::New();
  model_files->weights = std::move(*weights);
  model_files->weights_dense1 = std::move(*weights_dense1);
  model_files->weights_dense2 = std::move(*weights_dense2);
  model_files->tokenizer = std::move(*tokenizer);
  model_files->config = std::move(*config);
  return model_files;
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
  // AddObserver re-fires OnLocalModelsReady synchronously if the
  // component is already installed; our handler sets model_dir_ready_
  // and notifies observer_list_ via EmbedderMetadataUpdated.
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

void BravePassageEmbeddingsServiceController::MaybeLaunchService() {
  if (service_) {
    return;
  }
  service_ = std::make_unique<BravePassageEmbeddingsService>(
      base::BindRepeating(&CreateBackgroundWebContents));
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

void BravePassageEmbeddingsServiceController::BindLocalAIReceiver(
    mojo::PendingReceiver<local_ai::mojom::LocalAIService> receiver) {
  if (service_) {
    service_->BindLocalAIReceiver(std::move(receiver));
  }
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
  // Mirrors upstream's "do we have a model path to load from?" check
  // — true once LocalModelsUpdaterState reports the component is
  // installed. SchedulingEmbedder retries on the
  // EmbedderMetadataUpdated notification fired in OnLocalModelsReady.
  return model_dir_ready_;
}

void BravePassageEmbeddingsServiceController::OnLocalModelsReady(
    const base::FilePath& install_dir) {
  model_dir_ready_ = !install_dir.empty();
  if (!model_dir_ready_) {
    // Component uninstall (or test reset) cleared the dir. Tear the
    // service down if any so it doesn't outlive the model files.
    if (service_) {
      ResetServiceRemote();
    }
    return;
  }
  // SchedulingEmbedder is the only observer and SubmitWorkToEmbedder()
  // short-circuits if work is already in flight, so re-firing on
  // repeated component-updater notifications is harmless.
  observer_list_.Notify(&EmbedderMetadataObserver::EmbedderMetadataUpdated,
                        GetEmbedderMetadata());
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

  if (!EmbedderReady()) {
    DVLOG(1) << "GetEmbeddings called before model dir is ready";
    std::move(callback).Run({}, ComputeEmbeddingsStatus::kModelUnavailable);
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
    DVLOG(3) << "GetEmbeddings: posting model-file load for new embedder";
    LoadModelFilesAndBind(std::move(receiver));
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

void BravePassageEmbeddingsServiceController::LoadModelFilesAndBind(
    mojo::PendingReceiver<mojom::PassageEmbedder> receiver) {
  auto* updater_state = local_ai::LocalModelsUpdaterState::GetInstance();
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&LoadLocalModelFilesFromDisk,
                     updater_state->GetEmbeddingGemmaModel(),
                     updater_state->GetEmbeddingGemmaDense1(),
                     updater_state->GetEmbeddingGemmaDense2(),
                     updater_state->GetEmbeddingGemmaTokenizer(),
                     updater_state->GetEmbeddingGemmaConfig()),
      base::BindOnce(
          &BravePassageEmbeddingsServiceController::OnLocalModelFilesLoaded,
          weak_ptr_factory_.GetWeakPtr(), std::move(receiver)));
}

void BravePassageEmbeddingsServiceController::OnLocalModelFilesLoaded(
    mojo::PendingReceiver<mojom::PassageEmbedder> receiver,
    local_ai::mojom::ModelFilesPtr model_files) {
  if (!service_) {
    DVLOG(1) << "Service torn down before model files finished loading";
    return;
  }
  if (!model_files) {
    DVLOG(1) << "Model files load failed; tearing down service";
    // Receiver pipe is dropped here; embedder_remote_'s disconnect
    // handler will fire and call ResetServiceRemote.
    return;
  }
  service_->BindPassageEmbedder(
      std::move(receiver), std::move(model_files),
      base::BindOnce([](bool success) {
        DVLOG_IF(1, !success)
            << "BravePassageEmbeddingsService reported BindPassageEmbedder "
               "failure; this batch will return an empty result";
      }));
}

}  // namespace passage_embeddings
