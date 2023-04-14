/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_classification_resource.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/contextual/text_classification/text_classification_features.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/text_processing.h"
#include "brave/components/brave_ads/core/internal/resources/resources_util_impl.h"

namespace brave_ads::resource {

namespace {
constexpr char kResourceId[] = "feibnmjhecfbjpeciancnchbmlobenjn";
}  // namespace

TextClassification::TextClassification() = default;
TextClassification::~TextClassification() = default;

bool TextClassification::IsInitialized() const {
  return text_processing_pipeline_.IsInitialized();
}

void TextClassification::Load() {
  LoadAndParseResource(
      kResourceId, targeting::kTextClassificationResourceVersion.Get(),
      base::BindOnce(&TextClassification::OnLoadAndParseResource,
                     weak_factory_.GetWeakPtr()));
}

void TextClassification::OnLoadAndParseResource(
    ParsingErrorOr<ml::pipeline::TextProcessing> result) {
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

const ml::pipeline::TextProcessing* TextClassification::Get() const {
  return &text_processing_pipeline_;
}

}  // namespace brave_ads::resource
