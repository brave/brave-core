/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"

#include <string>

#include "base/files/file_util.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/features/text_classification/text_classification_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/ml/pipeline/text_processing/text_processing.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {
namespace resource {

namespace {
constexpr char kResourceId[] = "feibnmjhecfbjpeciancnchbmlobenjn";
}  // namespace

TextClassification::TextClassification() {
  text_processing_pipeline_.reset(
      ml::pipeline::TextProcessing::CreateInstance());
}

TextClassification::~TextClassification() = default;

bool TextClassification::IsInitialized() const {
  return text_processing_pipeline_ &&
         text_processing_pipeline_->IsInitialized();
}

void TextClassification::Load() {
  AdsClientHelper::Get()->LoadAdsFileResource(
      kResourceId, features::GetTextClassificationResourceVersion(),
      [=](base::File file) {
        text_processing_pipeline_.reset(
            ml::pipeline::TextProcessing::CreateInstance());

        if (!file.IsValid()) {
          BLOG(1, "Failed to load " << kResourceId
                                    << " text classification resource");
          return;
        }

        std::string json;
        // TODO(atuchin): move reading from utilty main to blocking pool.
        base::ScopedFILE stream(base::FileToFILE(std::move(file), "rb"));
        if (!base::ReadStreamToString(stream.get(), &json)) {
          return;
        }

        BLOG(1, "Successfully loaded " << kResourceId
                                       << " text classification resource");

        if (!text_processing_pipeline_->FromJson(std::move(json))) {
          BLOG(1, "Failed to initialize " << kResourceId
                                          << " text classification resource");
          return;
        }

        BLOG(1, "Successfully initialized " << kResourceId
                                            << " text classification resource");
      });
}

ml::pipeline::TextProcessing* TextClassification::get() const {
  return text_processing_pipeline_.get();
}

}  // namespace resource
}  // namespace ads
