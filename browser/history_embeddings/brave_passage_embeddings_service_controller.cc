// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_passage_embeddings_service_controller.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
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
#include "chrome/browser/profiles/profile.h"
#include "components/optimization_guide/proto/passage_embeddings_model_metadata.pb.h"
#include "components/passage_embeddings/core/passage_embeddings_service_launcher.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "content/public/browser/service_process_host.h"
#include "mojo/public/cpp/base/big_buffer.h"
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

// The three helpers immediately below (OnBackgroundWebContentsCreated,
// CreateBackgroundWebContents, LoadLocalModelFilesFromDisk) and the controller
// methods tagged "Dead WASM code" are no longer reached now that LiteRT is the
// only embedder. They are kept to keep this change minimal and are removed,
// along with the WASM embedder itself, in the follow-up.

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
    content::ServiceProcessHost::Launch<mojom::PassageEmbeddingsService>(
        std::move(receiver), content::ServiceProcessHost::Options()
                                 .WithDisplayName("Passage Embeddings Service")
                                 .Pass());
  }
  void OnServiceDisconnected(bool is_idle) override {}
  bool AllowedToLaunch() const override { return true; }
};

PassageEmbeddingsServiceLauncher& GetServiceLauncher() {
  return LitertServiceLauncher::Create();
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
  // Report the LiteRT model's metadata; the model file paths are resolved
  // from the component in OnLocalModelsReady once it is installed.
  optimization_guide::proto::PassageEmbeddingsModelMetadata metadata;
  metadata.set_input_window_size(kLitertInputWindowSize);
  metadata.set_output_size(kLitertOutputSize);
  metadata.set_score_threshold(kLitertScoreThreshold);
  model_metadata_ = std::move(metadata);
  model_version_ = kLitertModelVersion;

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

// Dead WASM code (MaybeLaunchService through OnProfileWillBeDestroyed): the
// in-process WASM service lifecycle, unreached now that LiteRT is the only
// embedder.
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
  // The LiteRT model paths are resolved from the component in
  // OnLocalModelsReady.
  return !embeddings_model_path_.empty();
}

void BravePassageEmbeddingsServiceController::OnLocalModelsReady(
    const base::FilePath& install_dir) {
  model_dir_ready_ = !install_dir.empty();
  if (!model_dir_ready_) {
    // Component uninstall (or test reset) cleared the dir. Drop the resolved
    // model paths and tear the service down if any so it doesn't outlive the
    // model files. (service_ is WASM/candle dead code and is never set now.)
    embeddings_model_path_ = base::FilePath();
    sp_model_path_ = base::FilePath();
    if (service_) {
      ResetServiceRemote();
    }
    return;
  }
  // The LiteRT .tflite + SentencePiece model ship in the EmbeddingGemma
  // component under a litert/ subdir. They may be absent (an older component,
  // or the download withheld), so confirm both are on disk before reporting the
  // model available -- the base-class LoadModels flow opens them
  // unconditionally and would crash serializing a missing (null) file.
  const base::FilePath litert_dir =
      local_ai::LocalModelsUpdaterState::GetInstance()
          ->GetEmbeddingGemmaModelDir()
          .AppendASCII(kLitertModelSubdir);
  const base::FilePath embeddings_model_path =
      litert_dir.AppendASCII(kLitertModelName);
  const base::FilePath sp_model_path =
      litert_dir.AppendASCII(kSentencePieceModelName);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(
          [](const base::FilePath& embeddings, const base::FilePath& sp) {
            return base::PathExists(embeddings) && base::PathExists(sp);
          },
          embeddings_model_path, sp_model_path),
      base::BindOnce(
          &BravePassageEmbeddingsServiceController::OnLitertModelChecked,
          base::Unretained(this), embeddings_model_path, sp_model_path));
}

void BravePassageEmbeddingsServiceController::OnLitertModelChecked(
    const base::FilePath& embeddings_model_path,
    const base::FilePath& sp_model_path,
    bool models_exist) {
  if (!models_exist) {
    VLOG(1) << "LiteRT model files missing under "
            << embeddings_model_path.DirName()
            << "; passage embeddings disabled until the component ships them";
    return;
  }
  embeddings_model_path_ = embeddings_model_path;
  sp_model_path_ = sp_model_path;
  // SchedulingEmbedder is the only observer and SubmitWorkToEmbedder()
  // short-circuits if work is already in flight, so re-firing on
  // repeated component-updater notifications is harmless.
  observer_list_.Notify(&EmbedderMetadataObserver::EmbedderMetadataUpdated,
                        GetEmbedderMetadata());
}

EmbedderMetadata
BravePassageEmbeddingsServiceController::GetEmbedderMetadata() {
  // Report a model_version distinct from the previous WASM embedder's so that
  // upstream drops the old embeddings rather than mixing vector spaces. On
  // startup SqlDatabase::InitInternal()
  // (//components/history_embeddings/core/sql_database.cc) compares this with
  // the stored model_version and, on a mismatch, clears the embeddings table
  // and re-embeds the retained passages.
  return EmbedderMetadata(kLitertModelVersion,
                          /*output_size=*/kLitertOutputSize,
                          /*search_score_threshold=*/kLitertScoreThreshold);
}

void BravePassageEmbeddingsServiceController::GetEmbeddings(
    std::vector<std::string> passages,
    PassagePriority priority,
    GetEmbeddingsResultCallback callback) {
  // Reuse the upstream launch + LoadModels flow, which spins up the sandboxed
  // Passage Embeddings utility process and drives the LiteRT embedder there.
  PassageEmbeddingsServiceController::GetEmbeddings(
      std::move(passages), priority, std::move(callback));
}

// Dead WASM code: LoadModelFilesAndBind + OnLocalModelFilesLoaded loaded model
// files for the in-process WASM embedder.
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
