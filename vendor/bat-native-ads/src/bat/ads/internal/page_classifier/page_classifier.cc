/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/page_classifier/page_classifier.h"

#include <algorithm>

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/page_classifier/page_classifier_util.h"

#include "base/logging.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {

PageClassifier::PageClassifier(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

PageClassifier::~PageClassifier() = default;

bool PageClassifier::IsInitialized() {
  return user_model_ && user_model_->IsInitialized();
}

bool PageClassifier::Initialize(
    const std::string& json) {
  user_model_.reset(usermodel::UserModel::CreateInstance());
  return user_model_->InitializePageClassifier(json);
}

bool PageClassifier::ShouldClassifyPages() const {
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();

  return ShouldClassifyPagesForLocale(locale);
}

std::string PageClassifier::ClassifyPage(
    const std::string& url,
    const std::string& content) {
  DCHECK(!url.empty());
  DCHECK(user_model_);

  const std::string normalized_content =
      page_classifier::NormalizeContent(content);

  const PageProbabilitiesMap page_probabilities =
      user_model_->ClassifyPage(normalized_content);

  const std::string page_classification =
      GetPageClassification(page_probabilities);

  if (!page_classification.empty()) {
    ads_->get_client()->AppendPageProbabilitiesToHistory(page_probabilities);
    CachePageProbabilities(url, page_probabilities);
  }

  return page_classification;
}

CategoryList PageClassifier::GetWinningCategories() const {
  CategoryList winning_categories;

  if (!ShouldClassifyPages()) {
    return winning_categories;
  }

  const PageProbabilitiesList page_probabilities =
      ads_->get_client()->GetPageProbabilitiesHistory();
  if (page_probabilities.empty()) {
    return winning_categories;
  }

  const CategoryProbabilitiesMap category_probabilities =
      GetCategoryProbabilities(page_probabilities);

  const CategoryProbabilitiesList winning_category_probabilities =
      GetWinningCategoryProbabilities(category_probabilities,
          kTopWinningCategoryCountForServingAds);

  winning_categories = ToCategoryList(winning_category_probabilities);

  return winning_categories;
}

const PageProbabilitiesCacheMap&
PageClassifier::get_page_probabilities_cache() const {
  return page_probabilities_cache_;
}

//////////////////////////////////////////////////////////////////////////////

bool PageClassifier::ShouldClassifyPagesForLocale(
    const std::string& locale) const {
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  const std::vector<std::string> user_model_languages =
      ads_->get_ads_client()->GetUserModelLanguages();

  const auto iter = std::find(user_model_languages.begin(),
      user_model_languages.end(), language_code);

  if (iter == user_model_languages.end()) {
    return false;
  }

  return true;
}

std::string PageClassifier::GetPageClassification(
    const PageProbabilitiesMap& page_probabilities) const {
  if (page_probabilities.empty()) {
    return "";
  }

  const auto iter = std::max_element(page_probabilities.begin(),
      page_probabilities.end(), [](const CategoryProbabilityPair& a,
          const CategoryProbabilityPair& b) -> bool {
    return a.second < b.second;
  });

  return iter->first;
}

CategoryProbabilitiesMap PageClassifier::GetCategoryProbabilities(
    const PageProbabilitiesList& page_probabilities) const {
  CategoryProbabilitiesMap category_probabilities;

  for (const auto& probabilities : page_probabilities) {
    for (const auto& probability : probabilities) {
      const std::string category = probability.first;
      if (ads_->get_client()->IsFilteredCategory(category)) {
        continue;
      }

      const double page_score = probability.second;

      const auto iter = category_probabilities.find(category);
      if (iter == category_probabilities.end()) {
        CategoryProbabilityPair category_probability = {category, page_score};
        category_probabilities.insert(category_probability);
      } else {
        iter->second += page_score;
      }
    }
  }

  return category_probabilities;
}

CategoryProbabilitiesList PageClassifier::GetWinningCategoryProbabilities(
    const CategoryProbabilitiesMap& category_probabilities,
    const int count) const {
  CategoryProbabilitiesList winning_category_probabilities(count);

  std::partial_sort_copy(category_probabilities.begin(),
      category_probabilities.end(), winning_category_probabilities.begin(),
          winning_category_probabilities.end(), [](
              const CategoryProbabilityPair& lhs,
                  const CategoryProbabilityPair& rhs) {
    return lhs.second > rhs.second;
  });

  return winning_category_probabilities;
}

void PageClassifier::CachePageProbabilities(
    const std::string& url,
    const PageProbabilitiesMap& page_probabilities) {
  if (page_probabilities.empty()) {
    return;
  }

  const auto iter = page_probabilities_cache_.find(url);
  if (iter == page_probabilities_cache_.end()) {
    page_probabilities_cache_.insert({url, page_probabilities});
  } else {
    iter->second = page_probabilities;
  }
}

CategoryList PageClassifier::ToCategoryList(
    const CategoryProbabilitiesList category_probabilities) const {
  CategoryList categories;

  for (const auto& category_probability : category_probabilities) {
    const std::string category = category_probability.first;
    categories.push_back(category);
  }

  return categories;
}

}  // namespace ads
