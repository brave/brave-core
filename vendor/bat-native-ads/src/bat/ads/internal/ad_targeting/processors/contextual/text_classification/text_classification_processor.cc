/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/processors/contextual/text_classification/text_classification_processor.h"

#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/ml/pipeline/text_processing/text_processing.h"

#include "third_party/cld_3/src/src/nnet_language_identifier.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"

//DEBUGGIN
#include <iostream>

namespace ads {
namespace ad_targeting {
namespace processor {

namespace {

std::string GetTopSegmentFromPageProbabilities(
    const TextClassificationProbabilitiesMap& probabilities) {
  if (probabilities.empty()) {
    return "";
  }

  const auto iter =
      std::max_element(probabilities.begin(), probabilities.end(),
                       [](const SegmentProbabilityPair& lhs,
                          const SegmentProbabilityPair& rhs) -> bool {
                         return lhs.second < rhs.second;
                       });

  return iter->first;
}

chrome_lang_id::NNetLanguageIdentifier::Result detect_page_language(const std::string& text) {

  const std::string contents = text;

  chrome_lang_id::NNetLanguageIdentifier lang_id;
  const chrome_lang_id::NNetLanguageIdentifier::Result lang_id_result =
      lang_id.FindTopNMostFreqLangs(contents, /*num_langs=*/1).at(0);

  return lang_id_result;
}

}  // namespace

TextClassification::TextClassification(resource::TextClassification* resource)
    : resource_(resource) {
  DCHECK(resource_);
}

TextClassification::~TextClassification() = default;

void TextClassification::Process(const std::string& text) {
  if (!resource_->IsInitialized()) {
    BLOG(1,
         "Failed to process text classification as resource "
         "not initialized");
    return;
  }

  const chrome_lang_id::NNetLanguageIdentifier::Result detected_lang = detect_page_language(text);
  std::cerr << "LANGUAGE DETECTOR:"
            << detected_lang.language << " " 
            << detected_lang.probability << "\n";

  const std::string locale = brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string language_code = brave_l10n::GetLanguageCode(locale);
  std::cerr << "LANGUAGE CODE FOR LOCALE:"
            << language_code << "for " 
            << locale << "\n";

  //TODO: Prevent classification if locale doesn't match detected language
  // if detection confidence not enough bypass and go with locale

  ml::pipeline::TextProcessing* text_proc_pipeline = resource_->get();

  const TextClassificationProbabilitiesMap probabilities =
      text_proc_pipeline->ClassifyPage(text);

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
