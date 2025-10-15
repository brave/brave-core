/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/optimization_guide/core/delivery/prediction_manager.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/path_service.h"
#include "components/optimization_guide/core/delivery/optimization_target_model_observer.h"
#include "components/optimization_guide/core/delivery/prediction_model_download_manager.h"
#include "components/optimization_guide/core/delivery/prediction_model_fetcher.h"
#include "components/optimization_guide/core/delivery/prediction_model_store.h"
#include "components/optimization_guide/core/optimization_guide_logger.h"
#include "components/optimization_guide/proto/models.pb.h"
#include "components/prefs/pref_service.h"
#include "components/services/unzip/public/cpp/unzip.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace optimization_guide {

namespace {
proto::ModelCacheKey GetModelCacheKey(const std::string& locale) {
  proto::ModelCacheKey model_cache_key;
  model_cache_key.set_locale(locale);
  return model_cache_key;
}
}  // namespace

PredictionManager::PredictionManager(
    PredictionModelStore* prediction_model_store,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_state,
    const std::string& application_locale,
    OptimizationGuideLogger* optimization_guide_logger,
    unzip::UnzipperFactory unzipper_factory)
    : registry_(optimization_guide_logger),
      prediction_model_store_(prediction_model_store),
      url_loader_factory_(url_loader_factory),
      optimization_guide_logger_(optimization_guide_logger),
      unzipper_factory_(std::move(unzipper_factory)),
      prediction_model_fetch_timer_(local_state, base::DoNothing()),
      application_locale_(application_locale),
      model_cache_key_(GetModelCacheKey(application_locale_)) {}

PredictionManager::~PredictionManager() {}

void PredictionManager::AddObserverForOptimizationTargetModel(
    proto::OptimizationTarget optimization_target,
    const std::optional<proto::Any>& model_metadata,
    scoped_refptr<base::SequencedTaskRunner> model_task_runner,
    OptimizationTargetModelObserver* observer) {}

void PredictionManager::RemoveObserverForOptimizationTargetModel(
    proto::OptimizationTarget optimization_target,
    OptimizationTargetModelObserver* observer) {}

void PredictionManager::SetPredictionModelFetcherForTesting(
    std::unique_ptr<PredictionModelFetcher> prediction_model_fetcher) {}

void PredictionManager::SetPredictionModelDownloadManagerForTesting(
    std::unique_ptr<PredictionModelDownloadManager>
        prediction_model_download_manager) {}

void PredictionManager::SetModelDownloadSchedulingParams(
    proto::OptimizationTarget optimization_target,
    const download::SchedulingParams& params) {}

base::flat_set<proto::OptimizationTarget>
PredictionManager::GetRegisteredOptimizationTargets() const {
  return base::flat_set<proto::OptimizationTarget>();
}

void PredictionManager::OverrideTargetModelForTesting(
    proto::OptimizationTarget optimization_target,
    std::unique_ptr<ModelInfo> model_info) {}

void PredictionManager::OnModelReady(const base::FilePath& base_model_dir,
                                     const proto::PredictionModel& model) {}

void PredictionManager::OnModelDownloadStarted(
    proto::OptimizationTarget optimization_target) {}

void PredictionManager::OnModelDownloadFailed(
    proto::OptimizationTarget optimization_target) {}

std::vector<optimization_guide_internals::mojom::DownloadedModelInfoPtr>
PredictionManager::GetDownloadedModelsInfoForWebUI() const {
  return std::vector<
      optimization_guide_internals::mojom::DownloadedModelInfoPtr>();
}

base::flat_map<std::string, bool>
PredictionManager::GetOnDeviceSupplementaryModelsInfoForWebUI() const {
  return base::flat_map<std::string, bool>();
}

void PredictionManager::MaybeInitializeModelDownloads(
    ProfileDownloadServiceTracker& profile_download_service_tracker,
    PrefService* local_state) {}

base::FilePath PredictionManager::GetBaseModelDirForDownload(
    proto::OptimizationTarget optimization_target) {
  return base::FilePath();
}

}  // namespace optimization_guide
