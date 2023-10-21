/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components.h"
#include "brave/components/brave_ads/core/internal/common/resources/resources_util_impl.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource_constants.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  return UserHasOptedInToNotificationAds();
}

}  // namespace

TextClassificationResource::TextClassificationResource() {
  AddAdsClientNotifierObserver(this);
}

TextClassificationResource::~TextClassificationResource() {
  RemoveAdsClientNotifierObserver(this);
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

  LoadAndParseResource(kTextClassificationResourceId,
                       kTextClassificationResourceVersion.Get(),
                       base::BindOnce(&TextClassificationResource::LoadCallback,
                                      weak_factory_.GetWeakPtr()));
}

void TextClassificationResource::LoadCallback(
    ResourceParsingErrorOr<ml::pipeline::TextProcessing> result) {
  if (!result.has_value()) {
    return BLOG(0, "Failed to initialize " << kTextClassificationResourceId
                                           << " text classification resource ("
                                           << result.error() << ")");
  }

  if (!result.value().IsInitialized()) {
    return BLOG(1, kTextClassificationResourceId
                       << " text classification resource is not available");
  }

  BLOG(1, "Successfully loaded " << kTextClassificationResourceId
                                 << " text classification resource");

  text_processing_pipeline_ = std::move(result).value();

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
    const std::string& /*locale=*/) {
  MaybeLoad();
}

void TextClassificationResource::OnNotifyPrefDidChange(
    const std::string& path) {
  if (path == brave_rewards::prefs::kEnabled ||
      path == prefs::kOptedInToNotificationAds) {
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

void TextClassificationResource::OnNotifyDidUnregisterResourceComponent(
    const std::string& id) {
  if (!IsValidLanguageComponentId(id)) {
    return;
  }

  manifest_version_.reset();

  Reset();
}

}  // namespace brave_ads
