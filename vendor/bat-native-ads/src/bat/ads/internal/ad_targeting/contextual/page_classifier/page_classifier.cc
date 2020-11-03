/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/contextual/page_classifier/page_classifier.h"

#include <functional>

#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_util.h"
#include "bat/ads/internal/ad_targeting/contextual/contextual_util.h"
#include "bat/ads/internal/ad_targeting/contextual/page_classifier/page_classifier_user_models.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/search_engine/search_providers.h"
#include "bat/ads/internal/url_util.h"

namespace ads {
namespace ad_targeting {
namespace contextual {

using std::placeholders::_1;
using std::placeholders::_2;

namespace {
const int kTopWinningCategoryCount = 3;
}  // namespace

PageClassifier::PageClassifier(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

PageClassifier::~PageClassifier() = default;

void PageClassifier::LoadUserModelForLocale(
    const std::string& locale) {
  const std::string language_code = brave_l10n::GetLanguageCode(locale);

  const auto iter = kPageClassificationLanguageCodes.find(language_code);
  if (iter == kPageClassificationLanguageCodes.end()) {
    BLOG(1, locale << " locale does not support page classification");
    user_model_.reset(usermodel::UserModel::CreateInstance());
    return;
  }

  LoadUserModelForId(iter->second);
}

void PageClassifier::LoadUserModelForId(
    const std::string& id) {
  auto callback =
      std::bind(&PageClassifier::OnLoadUserModelForId, this, id, _1, _2);
  ads_->get_ads_client()->LoadUserModelForId(id, callback);
}

std::string PageClassifier::MaybeClassifyPage(
    const std::string& url,
    const std::string& content) {
  if (!UrlHasScheme(url)) {
    BLOG(1, "Visited URL is not supported for page classification");
    return "";
  }

  if (SearchProviders::IsSearchEngine(url)) {
    BLOG(1, "Search engine pages are not supported for page classification");
    return "";
  }

  const std::string page_classification =
      ShouldClassifyPages() ? ClassifyPage(url, content) : kUntargeted;

  if (page_classification == kUntargeted) {
    const std::string locale =
        brave_l10n::LocaleHelper::GetInstance()->GetLocale();
    BLOG(1, locale << " locale does not support page classification");
    return page_classification;
  }

  if (page_classification.empty()) {
    BLOG(1, "Page not classified as not enough content");
    return "";
  }

  BLOG(1, "Classified page as " << page_classification);

  const CategoryList winning_categories = GetWinningCategories();
  if (winning_categories.empty()) {
    return page_classification;
  }

  BLOG(1, "Winning page classification over time is "
      << winning_categories.front());

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
          kTopWinningCategoryCount);

  winning_categories = ToCategoryList(winning_category_probabilities);

  return winning_categories;
}

const PageProbabilitiesCacheMap&
PageClassifier::get_page_probabilities_cache() const {
  return page_probabilities_cache_;
}

///////////////////////////////////////////////////////////////////////////////

bool PageClassifier::IsInitialized() const {
  return user_model_ && user_model_->IsInitialized();
}

bool PageClassifier::Initialize(
    const std::string& json) {
  user_model_.reset(usermodel::UserModel::CreateInstance());
  return user_model_->InitializePageClassifier(json);
}

void PageClassifier::OnLoadUserModelForId(
    const std::string& id,
    const Result result,
    const std::string& json) {
  if (result != SUCCESS) {
    BLOG(1, "Failed to load " << id << " page classification user model");
    user_model_.reset(usermodel::UserModel::CreateInstance());
    return;
  }

  BLOG(1, "Successfully loaded " << id << " page classification user model");

  if (!Initialize(json)) {
    BLOG(1, "Failed to initialize " << id << " page classification user model");
    user_model_.reset(usermodel::UserModel::CreateInstance());
    return;
  }

  BLOG(1, "Successfully initialized " << id << " page classification user "
      "model");
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

  const FilteredCategoryList filtered_categories =
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

}  // namespace contextual
}  // namespace ad_targeting
}  // namespace ads
