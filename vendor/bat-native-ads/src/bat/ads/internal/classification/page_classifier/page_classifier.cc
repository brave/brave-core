/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/classification/page_classifier/page_classifier.h"

#include <algorithm>

#include "base/logging.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/classification/classification_util.h"
#include "bat/ads/internal/classification/page_classifier/page_classifier_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace classification {

PageClassifier::PageClassifier(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

PageClassifier::~PageClassifier() = default;

bool PageClassifier::IsInitialized() const {
  return user_model_ && user_model_->IsInitialized();
}

bool PageClassifier::Initialize(
    const std::string& json) {
  user_model_.reset(usermodel::UserModel::CreateInstance());
  return user_model_->InitializePageClassifier(json);
}

bool PageClassifier::ShouldClassifyPages() const {
  return IsInitialized();
}

std::string PageClassifier::ClassifyPage(
    const std::string& url,
    const std::string& content) {
  DCHECK(!url.empty());
  DCHECK(user_model_);

  const std::string stripped_content =
      StripHtmlTagsAndNonAlphaCharacters(content);

  const PageProbabilitiesMap page_probabilities =
      user_model_->ClassifyPage(stripped_content);

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
      if (ShouldFilterCategory(category)) {
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

bool PageClassifier::ShouldFilterCategory(
    const std::string& category) const {
  // If passed in category has a subcategory and the current filter does not,
  // check if it's a child of the filter. Conversely, if the passed in category
  // has no subcategory but the current filter does, it can't be a match at all
  // so move on to the next filter. Otherwise, perform an exact match to
  // determine whether or not to filter the category

  const std::vector<std::string> category_classifications =
      SplitCategory(category);

  const FilteredCategoriesList filtered_categories =
      ads_->get_client()->get_filtered_categories();

  for (const auto& filtered_category : filtered_categories) {
    const std::vector<std::string> filtered_category_classifications =
        SplitCategory(filtered_category.name);

    if (category_classifications.size() > 1 &&
        filtered_category_classifications.size() == 1) {
      if (category_classifications.front() ==
          filtered_category_classifications.front()) {
        return true;
      }
    } else if (category_classifications.size() == 1 &&
        filtered_category_classifications.size() > 1) {
      continue;
    } else if (filtered_category.name == category) {
      return true;
    }
  }

  return false;
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

}  // namespace classification
}  // namespace ads
