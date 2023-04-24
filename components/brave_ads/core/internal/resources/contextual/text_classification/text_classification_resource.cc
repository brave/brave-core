/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_classification_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_feature.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"
#include "brave/components/brave_ads/core/internal/resources/resources_util_impl.h"

namespace brave_ads {

namespace {
constexpr char kResourceId[] = "feibnmjhecfbjpeciancnchbmlobenjn";
}  // namespace

TextClassificationResource::TextClassificationResource() = default;

TextClassificationResource::~TextClassificationResource() = default;

bool TextClassificationResource::IsInitialized() const {
  return text_processing_pipeline_.IsInitialized();
}

void TextClassificationResource::Load() {
  LoadAndParseResource(
      kResourceId, kTextClassificationResourceVersion.Get(),
      base::BindOnce(&TextClassificationResource::OnLoadAndParseResource,
                     weak_factory_.GetWeakPtr()));
}

void TextClassificationResource::OnLoadAndParseResource(
    ResourceParsingErrorOr<ml::pipeline::TextProcessing> result) {
  if (!result.has_value()) {
    BLOG(1, result.error());
    BLOG(1, "Failed to initialize " << kResourceId
                                    << " text classification resource");
    return;
  }
  BLOG(1, "Successfully loaded " << kResourceId
                                 << " text classification resource");
  text_processing_pipeline_ = std::move(result).value();

  BLOG(1, "Successfully initialized " << kResourceId
                                      << " text classification resource");
}

const ml::pipeline::TextProcessing* TextClassificationResource::Get() const {
  return &text_processing_pipeline_;
}

}  // namespace brave_ads
