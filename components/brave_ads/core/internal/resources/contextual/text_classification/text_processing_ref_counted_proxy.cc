/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_processing_ref_counted_proxy.h"

namespace brave_ads {

TextProcessingRefCountedProxy::TextProcessingRefCountedProxy() = default;

TextProcessingRefCountedProxy::~TextProcessingRefCountedProxy() = default;

TextClassificationProbabilityMap TextProcessingRefCountedProxy::ClassifyPage(
    const std::string& text) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const absl::optional<ml::pipeline::TextProcessing>& resource = GetResource();

  if (!resource || !resource->IsInitialized()) {
    return {};
  }

  const TextClassificationProbabilityMap probabilities =
      resource->ClassifyPage(text);
  return probabilities;
}

}  // namespace brave_ads
