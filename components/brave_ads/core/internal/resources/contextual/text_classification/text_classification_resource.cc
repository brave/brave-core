/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_classification_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/types/expected.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_classification_resource_constants.h"
#include "brave/components/brave_ads/core/internal/resources/language_components.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  return UserHasOptedInToBravePrivateAds();
}

}  // namespace

TextClassificationResource::TextClassificationResource() {
  AdsClientHelper::AddObserver(this);
}

TextClassificationResource::~TextClassificationResource() {
  AdsClientHelper::RemoveObserver(this);
}

void TextClassificationResource::ClassifyPage(const std::string& text,
                                              ClassifyPageCallback callback) {
  if (!DidLoad() || !IsInitialized()) {
    BLOG(1,
         "Failed to process text classification as resource not initialized");
    return std::move(callback).Run({});
  }

  text_processing_pipeline_->Get()
      .AsyncCall(&TextProcessingRefCountedProxy::ClassifyPage)
      .WithArgs(text)
      .Then(std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void TextClassificationResource::MaybeLoad() {
  if (manifest_version_ && DoesRequireResource()) {
    Load();
  }
}

void TextClassificationResource::MaybeLoadOrReset() {
  DidLoad() ? MaybeReset() : MaybeLoad();
}

void TextClassificationResource::Load() {
  did_load_ = true;

  AdsClientHelper::GetInstance()->LoadFileResource(
      kTextClassificationResourceId, kTextClassificationResourceVersion.Get(),
      base::BindOnce(&TextClassificationResource::OnLoadFileResource,
                     weak_factory_.GetWeakPtr()));
}

void TextClassificationResource::OnLoadFileResource(base::File file) {
  if (!file.IsValid() || !manifest_version_) {
    return;
  }

  text_processing_pipeline_.emplace();
  text_processing_pipeline_->Get()
      .AsyncCall(&TextProcessingRefCountedProxy::Load)
      .WithArgs(std::move(file), *manifest_version_)
      .Then(base::BindOnce(&TextClassificationResource::LoadCallback,
                           weak_factory_.GetWeakPtr()));
}

void TextClassificationResource::LoadCallback(
    base::expected<bool, std::string> result) {
  if (!result.has_value()) {
    BLOG(0, "Failed to initialize " << kTextClassificationResourceId
                                    << " text classification resource ("
                                    << result.error() << ")");
    return text_processing_pipeline_.reset();
  }

  if (!result.value()) {
    BLOG(1, kTextClassificationResourceId
                << " text classification resource is not available");
    return text_processing_pipeline_.reset();
  }

  BLOG(1, "Successfully loaded " << kTextClassificationResourceId
                                 << " text classification resource");

  BLOG(1, "Successfully initialized "
              << kTextClassificationResourceId
              << " text classification resource version "
              << kTextClassificationResourceVersion.Get());
}

void TextClassificationResource::MaybeReset() {
  if (DidLoad() && !DoesRequireResource()) {
    Reset();
  }
}

void TextClassificationResource::Reset() {
  BLOG(1, "Reset " << kTextClassificationResourceId
                   << " text classification resource");
  text_processing_pipeline_.reset();
  did_load_ = false;
}

void TextClassificationResource::OnNotifyLocaleDidChange(
    const std::string& /*locale*/) {
  MaybeLoad();
}

void TextClassificationResource::OnNotifyPrefDidChange(
    const std::string& path) {
  if (path == prefs::kEnabled) {
    MaybeLoadOrReset();
  }
}

void TextClassificationResource::OnNotifyDidUpdateResourceComponent(
    const std::string& manifest_version,
    const std::string& id) {
  if (!IsValidLanguageComponentId(id)) {
    return;
  }

  if (manifest_version == manifest_version_) {
    return;
  }

  manifest_version_ = manifest_version;

  MaybeLoad();
}

}  // namespace brave_ads
