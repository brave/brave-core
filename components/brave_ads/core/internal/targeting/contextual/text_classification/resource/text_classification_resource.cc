/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_path_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource_constants.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  // Require resource only if:
  // - The user has joined Brave Rewards and opted into notification ads.
  return UserHasOptedInToNotificationAds();
}

}  // namespace

TextClassificationResource::TextClassificationResource() {
  GetAdsClient().AddObserver(this);
}

TextClassificationResource::~TextClassificationResource() {
  GetAdsClient().RemoveObserver(this);
}

void TextClassificationResource::ClassifyPage(const std::string& text,
                                              ClassifyPageCallback callback) {
  if (!IsLoaded()) {
    BLOG(1, "Failed to process text classification as resource not loaded");

    return std::move(callback).Run(/*probabilities=*/{});
  }

  text_processing_pipeline_
      ->AsyncCall(&ml::pipeline::TextProcessing::ClassifyPage)
      .WithArgs(text)
      .Then(std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void TextClassificationResource::MaybeLoad() {
  if (manifest_version_ && DoesRequireResource()) {
    Load();
  }
}

void TextClassificationResource::MaybeLoadOrUnload() {
  IsLoaded() ? MaybeUnload() : MaybeLoad();
}

void TextClassificationResource::Load() {
  GetAdsClient().LoadResourceComponent(
      kTextClassificationResourceId, kTextClassificationResourceVersion.Get(),
      base::BindOnce(&TextClassificationResource::LoadResourceComponentCallback,
                     weak_factory_.GetWeakPtr()));
}

void TextClassificationResource::LoadResourceComponentCallback(
    base::File file) {
  if (!file.IsValid()) {
    return BLOG(0, "Failed to load " << kTextClassificationResourceId
                                     << " text classification resource");
  }

  text_processing_pipeline_.emplace(
      base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}));
  text_processing_pipeline_
      ->AsyncCall(&ml::pipeline::TextProcessing::LoadPipeline)
      .WithArgs(std::move(file))
      .Then(base::BindOnce(&TextClassificationResource::LoadCallback,
                           weak_factory_.GetWeakPtr()));
}

void TextClassificationResource::LoadCallback(
    base::expected<bool, std::string> result) {
  if (!result.has_value()) {
    text_processing_pipeline_.reset();

    return BLOG(0, "Failed to load " << kTextClassificationResourceId
                                     << " text classification resource ("
                                     << result.error() << ")");
  }

  BLOG(1, "Successfully loaded "
              << kTextClassificationResourceId << " "
              << (/*is_neural*/ result.value() ? "neural" : "linear")
              << " text classification resource version "
              << kTextClassificationResourceVersion.Get());
}

void TextClassificationResource::MaybeUnload() {
  if (manifest_version_ && !DoesRequireResource()) {
    Unload();
  }
}

void TextClassificationResource::Unload() {
  BLOG(1, "Unloaded " << kTextClassificationResourceId
                      << " text classification resource");

  text_processing_pipeline_.reset();
}

void TextClassificationResource::OnNotifyLocaleDidChange(
    const std::string& /*locale*/) {
  MaybeLoad();
}

void TextClassificationResource::OnNotifyPrefDidChange(
    const std::string& path) {
  if (DoesMatchUserHasJoinedBraveRewardsPrefPath(path) ||
      DoesMatchUserHasOptedInToNotificationAdsPrefPath(path)) {
    // This condition should include all the preferences that are present in the
    // `DoesRequireResource` function.
    MaybeLoadOrUnload();
  }
}

void TextClassificationResource::OnNotifyResourceComponentDidChange(
    const std::string& manifest_version,
    const std::string& id) {
  if (!IsValidLanguageComponentId(id)) {
    return;
  }

  if (manifest_version == manifest_version_) {
    // No need to load the resource if the manifest version is the same.
    return;
  }

  if (!manifest_version_) {
    BLOG(1, "Registering "
                << id
                << " text classification resource component manifest version "
                << manifest_version);
  } else {
    BLOG(1, "Updating "
                << id
                << " text classification resource component manifest version "
                << *manifest_version_ << " to " << manifest_version);
  }

  manifest_version_ = manifest_version;

  MaybeLoad();
}

void TextClassificationResource::OnNotifyDidUnregisterResourceComponent(
    const std::string& id) {
  if (!IsValidLanguageComponentId(id)) {
    return;
  }

  BLOG(1, "Unregistering " << id << " text classification resource component");

  manifest_version_.reset();

  Unload();
}

}  // namespace brave_ads
