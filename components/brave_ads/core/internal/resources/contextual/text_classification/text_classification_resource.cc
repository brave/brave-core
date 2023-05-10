/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_classification_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"
#include "brave/components/brave_ads/core/internal/resources/language_components.h"
#include "brave/components/brave_ads/core/internal/resources/resources_util_impl.h"

namespace brave_ads {

namespace {
constexpr char kResourceId[] = "feibnmjhecfbjpeciancnchbmlobenjn";
}  // namespace

TextClassificationResource::TextClassificationResource() {
  AdsClientHelper::AddObserver(this);
}

TextClassificationResource::~TextClassificationResource() {
  AdsClientHelper::RemoveObserver(this);
}

bool TextClassificationResource::IsInitialized() const {
  return text_processing_pipeline_.IsInitialized();
}

void TextClassificationResource::Load() {
  LoadAndParseResource(
      kResourceId, kTextClassificationResourceVersion.Get(),
      base::BindOnce(&TextClassificationResource::LoadAndParseResourceCallback,
                     weak_factory_.GetWeakPtr()));
}

///////////////////////////////////////////////////////////////////////////////

void TextClassificationResource::LoadAndParseResourceCallback(
    ResourceParsingErrorOr<ml::pipeline::TextProcessing> result) {
  if (!result.has_value()) {
    BLOG(0, "Failed to initialize " << kResourceId
                                    << " text classification resource ("
                                    << result.error() << ")");
    return;
  }

  if (!result.value().IsInitialized()) {
    BLOG(7, kResourceId << " text classification resource does not exist");
    return;
  }

  BLOG(1, "Successfully loaded " << kResourceId
                                 << " text classification resource");

  text_processing_pipeline_ = std::move(result).value();

  BLOG(1, "Successfully initialized "
              << kResourceId << " text classification resource version "
              << kTextClassificationResourceVersion.Get());
}

void TextClassificationResource::OnNotifyLocaleDidChange(
    const std::string& /*locale*/) {
  Load();
}

void TextClassificationResource::OnNotifyDidUpdateResourceComponent(
    const std::string& id) {
  if (IsValidLanguageComponentId(id)) {
    Load();
  }
}

}  // namespace brave_ads
