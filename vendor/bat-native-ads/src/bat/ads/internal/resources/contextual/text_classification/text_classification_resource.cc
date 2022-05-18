/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"

#include <utility>

#include "base/bind.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/ml/pipeline/text_processing/text_processing.h"
#include "bat/ads/internal/resources/resources_util_impl.h"
#include "bat/ads/internal/serving/targeting/models/contextual/text_classification/text_classification_features.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {
namespace resource {

namespace {
constexpr char kResourceId[] = "feibnmjhecfbjpeciancnchbmlobenjn";
}  // namespace

TextClassification::TextClassification()
    : text_processing_pipeline_(
          std::make_unique<ml::pipeline::TextProcessing>()) {}

TextClassification::~TextClassification() = default;

bool TextClassification::IsInitialized() const {
  return text_processing_pipeline_ &&
         text_processing_pipeline_->IsInitialized();
}

void TextClassification::Load() {
  LoadAndParseResource(
      kResourceId, features::GetTextClassificationResourceVersion(),
      base::BindOnce(&TextClassification::OnLoadAndParseResource,
                     weak_ptr_factory_.GetWeakPtr()));
}

void TextClassification::OnLoadAndParseResource(
    ParsingResultPtr<ml::pipeline::TextProcessing> result) {
  if (!result) {
    BLOG(1,
         "Failed to load " << kResourceId << " text classification resource");
    return;
  }
  BLOG(1, "Successfully loaded " << kResourceId
                                 << " text classification resource");

  if (!result->resource) {
    BLOG(1, result->error_message);
    BLOG(1, "Failed to initialize " << kResourceId
                                    << " text classification resource");
    return;
  }

  text_processing_pipeline_ = std::move(result->resource);

  BLOG(1, "Successfully initialized " << kResourceId
                                      << " text classification resource");
}

ml::pipeline::TextProcessing* TextClassification::get() const {
  return text_processing_pipeline_.get();
}

}  // namespace resource
}  // namespace ads
