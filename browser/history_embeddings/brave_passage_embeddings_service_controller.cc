// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/browser/history_embeddings/brave_batch_passage_embedder.h"
#include "brave/browser/local_ai/background_web_contents_factory.h"
#include "brave/components/local_ai/core/local_models_updater.h"
#include "brave/components/local_ai/core/url_constants.h"
#include "brave/components/local_ai/core/utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "components/optimization_guide/proto/passage_embeddings_model_metadata.pb.h"
#include "components/passage_embeddings/core/passage_embeddings_features.h"
#include "components/passage_embeddings/core/passage_embeddings_service_launcher.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "content/public/browser/service_process_host.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "url/gurl.h"

namespace passage_embeddings {

namespace {

// The LiteRT model files ship in the shared EmbeddingGemma component (the same
// one the WASM embedder uses), under a litert/ subdir of its model dir.
constexpr char kLitertModelSubdir[] = "litert";
constexpr char kLitertModelName[] =
    "embeddinggemma-300M_seq512_mixed-precision.tflite";
constexpr char kSentencePieceModelName[] = "sentencepiece.model";
// Matches the seq512 model's input window. The embedder itself derives the
// actual window from the model's input tensor, so this only feeds the metadata
// the controller reports; keep it in sync with the model above.
constexpr uint32_t kLitertInputWindowSize = 512;
constexpr int kLitertOutputSize = 768;
constexpr double kLitertScoreThreshold = 0.45;
constexpr int kLitertModelVersion = 2;

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

// Completion callback for local_ai::CreateBackgroundWebContents().
//
// Registers the OTR profile with the controller so we tear the service down
// before the profile is destroyed on shutdown (otherwise the WebContents inside
// the embedder would outlive its BrowserContext), then forwards the
// BackgroundWebContents to the embedder.
void OnBackgroundWebContentsCreated(
    BraveBatchPassageEmbedder::BackgroundWebContentsCreatedCallback callback,
    std::unique_ptr<local_ai::BackgroundWebContents> contents,
    Profile* otr_profile) {
  if (otr_profile) {
    BravePassageEmbeddingsServiceController::Get()->ObserveGuestOTRProfile(
        otr_profile);
  }
  std::move(callback).Run(std::move(contents));
}

void CreateBackgroundWebContents(
    local_ai::BackgroundWebContents::Delegate* delegate,
    BraveBatchPassageEmbedder::BackgroundWebContentsCreatedCallback callback) {
  local_ai::CreateBackgroundWebContents(
      GURL(local_ai::kUntrustedLocalAIURL), IDS_LOCAL_AI_TASK_MANAGER_TITLE,
      /*sandbox_flags=*/std::nullopt,
      static_cast<BraveBatchPassageEmbedder*>(delegate)->GetWeakPtr(),
      base::BindOnce(&OnBackgroundWebContentsCreated, std::move(callback)));
}

local_ai::mojom::ModelFilesPtr LoadLocalModelFilesFromDisk(
    const base::FilePath& weights_path,
    const base::FilePath& weights_dense1_path,
    const base::FilePath& weights_dense2_path,
    const base::FilePath& tokenizer_path,
    const base::FilePath& config_path) {
  auto weights = local_ai::ReadFileToBigBuffer(weights_path);
  if (!weights) {
    return nullptr;
  }
  auto weights_dense1 = local_ai::ReadFileToBigBuffer(weights_dense1_path);
  if (!weights_dense1) {
    return nullptr;
  }
  auto weights_dense2 = local_ai::ReadFileToBigBuffer(weights_dense2_path);
  if (!weights_dense2) {
    return nullptr;
  }
  auto tokenizer = local_ai::ReadFileToBigBuffer(tokenizer_path);
  if (!tokenizer) {
    return nullptr;
  }
  auto config = local_ai::ReadFileToBigBuffer(config_path);
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

// `StubServiceLauncher` is provided to `PassageEmbeddingsServiceController`,
// and is used to spin up a sandboxed process. We pass a stub because we manage
// the embedding service in-process, and the launcher path is never taken.
class StubServiceLauncher : public PassageEmbeddingsServiceLauncher {
 public:
  static PassageEmbeddingsServiceLauncher& Create() {
    static base::NoDestructor<StubServiceLauncher> launcher;
    return *launcher;
  }

  void LaunchService(mojo::PendingReceiver<mojom::PassageEmbeddingsService>
                         receiver) override {
    NOTREACHED();
  }
  void OnServiceDisconnected(bool is_idle) override { NOTREACHED(); }
  bool AllowedToLaunch() const override { return false; }
};

// Launches the real sandboxed Passage Embeddings utility process. Used for the
// native LiteRT embedder, whose LoadModels is chromium_src-overridden to run
// EmbeddingGemma on LiteRT's CompiledModel inside that process.
class LitertServiceLauncher : public PassageEmbeddingsServiceLauncher {
 public:
  static PassageEmbeddingsServiceLauncher& Create() {
    static base::NoDestructor<LitertServiceLauncher> launcher;
    return *launcher;
  }

  void LaunchService(mojo::PendingReceiver<mojom::PassageEmbeddingsService>
                         receiver) override {
    content::ServiceProcessHost::Options options;
    options.WithDisplayName("Passage Embeddings Service");
#if BUILDFLAG(IS_MAC)
    // Scopes the ODME sandbox's user-dir write access to just this utility
    // (not the on-device model service), so LiteRT's GPU program cache and the
    // macOS Metal shader cache persist across launches. Read in content's
    // SetupGpuSandboxParameters; keep the switch name in sync. macOS-only: the
    // switch feeds a macOS sandbox parameter and is a no-op elsewhere.
    options.WithExtraCommandLineSwitches({"passage-embeddings-gpu-cache"});
#endif
    content::ServiceProcessHost::Launch<mojom::PassageEmbeddingsService>(
        std::move(receiver), options.Pass());
  }
  void OnServiceDisconnected(bool is_idle) override {}
  bool AllowedToLaunch() const override { return true; }
};

PassageEmbeddingsServiceLauncher& GetServiceLauncher() {
  return BravePassageEmbeddingsService::ShouldUseLitertEmbedder()
             ? LitertServiceLauncher::Create()
             : StubServiceLauncher::Create();
}

}  // namespace

// static
BravePassageEmbeddingsServiceController*
BravePassageEmbeddingsServiceController::Get() {
  static base::NoDestructor<BravePassageEmbeddingsServiceController> instance;
  return instance.get();
}

BravePassageEmbeddingsServiceController::
    BravePassageEmbeddingsServiceController()
    : PassageEmbeddingsServiceController(GetServiceLauncher()) {
  if (BravePassageEmbeddingsService::ShouldUseLitertEmbedder()) {
    // Report the LiteRT model's metadata; the model file paths are resolved
    // from the component in OnLocalModelsReady once it is installed.
    optimization_guide::proto::PassageEmbeddingsModelMetadata metadata;
    metadata.set_input_window_size(kLitertInputWindowSize);
    metadata.set_output_size(kLitertOutputSize);
    metadata.set_score_threshold(kLitertScoreThreshold);
    model_metadata_ = std::move(metadata);
    model_version_ = kLitertModelVersion;
  }

  // AddObserver re-fires OnLocalModelsReady synchronously if the component is
  // already installed; our handler sets the model paths / model_dir_ready_ and
  // notifies observer_list_ via EmbedderMetadataUpdated.
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

bool BravePassageEmbeddingsServiceController::IsModelAvailable() {
  if (BravePassageEmbeddingsService::ShouldUseLitertEmbedder()) {
    // The LiteRT model paths are resolved from the component in
    // OnLocalModelsReady.
    return !embeddings_model_path_.empty();
  }
  // Mirrors upstream's "do we have a model path to load from?" check
  // — true once LocalModelsUpdaterState reports the component is
  // installed. SchedulingEmbedder retries on the
  // EmbedderMetadataUpdated notification fired in OnLocalModelsReady.
  return model_dir_ready_;
}

void BravePassageEmbeddingsServiceController::OnLocalModelsReady(
    const base::FilePath& install_dir) {
  model_dir_ready_ = !install_dir.empty();
  const bool use_litert =
      BravePassageEmbeddingsService::ShouldUseLitertEmbedder();
  if (!model_dir_ready_) {
    // Component uninstall (or test reset) cleared the dir. Drop the resolved
    // model paths and tear the service down if any so it doesn't outlive the
    // model files.
    if (use_litert) {
      embeddings_model_path_ = base::FilePath();
      sp_model_path_ = base::FilePath();
    }
    if (service_) {
      ResetServiceRemote();
    }
    return;
  }
  if (use_litert) {
    // The LiteRT .tflite + SentencePiece model ship in the EmbeddingGemma
    // component under a litert/ subdir; the base-class launch + LoadModels flow
    // opens them and sends them to the utility process.
    const base::FilePath litert_dir =
        local_ai::LocalModelsUpdaterState::GetInstance()
            ->GetEmbeddingGemmaModelDir()
            .AppendASCII(kLitertModelSubdir);
    embeddings_model_path_ = litert_dir.AppendASCII(kLitertModelName);
    sp_model_path_ = litert_dir.AppendASCII(kSentencePieceModelName);
  }
  // SchedulingEmbedder is the only observer and SubmitWorkToEmbedder()
  // short-circuits if work is already in flight, so re-firing on
  // repeated component-updater notifications is harmless.
  observer_list_.Notify(&EmbedderMetadataObserver::EmbedderMetadataUpdated,
                        GetEmbedderMetadata());
}

EmbedderMetadata
BravePassageEmbeddingsServiceController::GetEmbedderMetadata() {
  // Use a distinct model_version for the native LiteRT embedder so that
  // switching to (or away from) it invalidates embeddings computed by the WASM
  // embedder -- SqlDatabase re-embeds stored history rather than mixing vector
  // spaces from two different backends.
  const int model_version =
      BravePassageEmbeddingsService::ShouldUseLitertEmbedder() ? 2 : 1;
  return EmbedderMetadata(model_version,
                          /*output_size=*/768,
                          /*search_score_threshold=*/0.45);
}

void BravePassageEmbeddingsServiceController::GetEmbeddings(
    std::vector<std::string> passages,
    PassagePriority priority,
    GetEmbeddingsResultCallback callback) {
  if (BravePassageEmbeddingsService::ShouldUseLitertEmbedder()) {
    // Reuse the upstream launch + LoadModels flow, which spins up the sandboxed
    // Passage Embeddings utility process and drives the LiteRT embedder there.
    PassageEmbeddingsServiceController::GetEmbeddings(
        std::move(passages), priority, std::move(callback));
    return;
  }

  if (passages.empty()) {
    std::move(callback).Run({}, ComputeEmbeddingsStatus::kSuccess);
    return;
  }

  if (!IsModelAvailable()) {
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
