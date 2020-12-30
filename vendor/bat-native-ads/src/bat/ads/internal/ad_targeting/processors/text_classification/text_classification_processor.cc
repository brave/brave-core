/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/processors/text_classification/text_classification_processor.h"

#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"
#include "bat/usermodel/user_model.h"

namespace ads {
namespace ad_targeting {
namespace processor {

namespace {

std::string GetTopSegmentFromPageProbabilities(
    const TextClassificationProbabilitiesMap& probabilities) {
  if (probabilities.empty()) {
    return "";
  }

  const auto iter = std::max_element(probabilities.begin(), probabilities.end(),
      [](const SegmentProbabilityPair& a,
          const SegmentProbabilityPair& b) -> bool {
    return a.second < b.second;
  });

  return iter->first;
}

}  // namespace

TextClassification::TextClassification(
    resource::TextClassification* resource)
    : resource_(resource) {
  DCHECK(resource_);
}

TextClassification::~TextClassification() = default;

void TextClassification::Process(
    const std::string& text) {
  if (!resource_->IsInitialized()) {
    BLOG(1, "Failed to process text classification as user model "
        "not initialized");
    return;
  }

  usermodel::UserModel* user_model = resource_->get();

  const TextClassificationProbabilitiesMap probabilities =
      user_model->ClassifyPage(text);

  if (probabilities.empty()) {
    BLOG(1, "Text not classified as not enough content");
    return;
  }

  const std::string segment = GetTopSegmentFromPageProbabilities(probabilities);
  BLOG(1, "Classified text with the top segment as " << segment);

  Client::Get()->AppendTextClassificationProbabilitiesToHistory(probabilities);
}

}  // namespace processor
}  // namespace ad_targeting
}  // namespace ads
