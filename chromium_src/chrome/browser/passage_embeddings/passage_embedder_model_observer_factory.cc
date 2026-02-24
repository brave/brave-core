/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/passage_embeddings/passage_embedder_model_observer_factory.h"

#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/optimization_guide/optimization_guide_keyed_service_factory.h"
#include "chrome/browser/passage_embeddings/chrome_passage_embeddings_service_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/optimization_guide/core/delivery/model_info.h"
#include "components/optimization_guide/core/delivery/model_util.h"
#include "components/optimization_guide/core/optimization_guide_proto_util.h"
#include "components/optimization_guide/proto/passage_embeddings_model_metadata.pb.h"
#include "components/passage_embeddings/passage_embedder_model_observer.h"
#include "components/passage_embeddings/passage_embeddings_features.h"

namespace passage_embeddings {

namespace {

void LoadLocalModelFiles(
    PassageEmbeddingsServiceController* controller) {
  base::FilePath home_dir;
  if (!base::PathService::Get(base::DIR_HOME, &home_dir)) {
    LOG(ERROR) << "Failed to get home directory";
    return;
  }

  base::FilePath model_dir =
      home_dir.Append(FILE_PATH_LITERAL("Downloads"))
          .Append(FILE_PATH_LITERAL("embedding-gemma-tflite"));
  base::FilePath tflite_path = model_dir.Append(FILE_PATH_LITERAL(
      "embeddinggemma-300M_seq2048_mixed-precision.tflite"));
  base::FilePath sp_path =
      model_dir.Append(FILE_PATH_LITERAL("sentencepiece.model"));

  optimization_guide::proto::PassageEmbeddingsModelMetadata metadata;
  metadata.set_input_window_size(2048);
  metadata.set_output_size(768);
  metadata.set_score_threshold(0.45);

  optimization_guide::proto::PredictionModel prediction_model;
  auto* model_info = prediction_model.mutable_model_info();
  model_info->set_version(1);
  *model_info->mutable_model_metadata() =
      optimization_guide::AnyWrapProto(metadata);

  auto* additional_file = model_info->add_additional_files();
  additional_file->set_file_path(
      optimization_guide::FilePathToString(sp_path));

  prediction_model.mutable_model()->set_download_url(
      optimization_guide::FilePathToString(tflite_path));

  auto model_info_obj =
      optimization_guide::ModelInfo::Create(prediction_model);
  if (!model_info_obj) {
    LOG(ERROR) << "Failed to create ModelInfo from local files";
    return;
  }

  if (!controller->MaybeUpdateModelInfo(*model_info_obj)) {
    LOG(ERROR) << "Failed to update model info on controller";
  }
}

}  // namespace

// static
PassageEmbedderModelObserver*
PassageEmbedderModelObserverFactory::GetForProfile(Profile* profile) {
  return static_cast<PassageEmbedderModelObserver*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create=*/true));
}

// static
PassageEmbedderModelObserverFactory*
PassageEmbedderModelObserverFactory::GetInstance() {
  static base::NoDestructor<PassageEmbedderModelObserverFactory> instance;
  return instance.get();
}

PassageEmbedderModelObserverFactory::PassageEmbedderModelObserverFactory()
    : ProfileKeyedServiceFactory(
          "HistoryEmbeddingsService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .WithAshInternals(ProfileSelection::kOriginalOnly)
              .Build()) {
  DependsOn(OptimizationGuideKeyedServiceFactory::GetInstance());
}

PassageEmbedderModelObserverFactory::~PassageEmbedderModelObserverFactory() =
    default;

std::unique_ptr<KeyedService>
PassageEmbedderModelObserverFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto observer = std::make_unique<PassageEmbedderModelObserver>(
      /*model_provider=*/nullptr,
      ChromePassageEmbeddingsServiceController::Get());

  LoadLocalModelFiles(ChromePassageEmbeddingsServiceController::Get());

  return observer;
}

}  // namespace passage_embeddings
