/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_PAGE_CLASSIFIER_PAGE_CLASSIFIER_H_
#define BAT_ADS_INTERNAL_PAGE_CLASSIFIER_PAGE_CLASSIFIER_H_

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bat/usermodel/user_model.h"

namespace ads {

using PageProbabilitiesMap = std::map<std::string, double>;
using PageProbabilitiesList = std::deque<PageProbabilitiesMap>;
using PageProbabilitiesCacheMap = std::map<std::string, PageProbabilitiesMap>;

using CategoryProbabilityPair = std::pair<std::string, double>;
using CategoryProbabilitiesList = std::vector<CategoryProbabilityPair>;
using CategoryProbabilitiesMap = std::map<std::string, double>;

using CategoryList = std::vector<std::string>;

class AdsImpl;

class PageClassifier {
 public:
  PageClassifier(
      const AdsImpl* const ads);

  ~PageClassifier();

  bool IsInitialized();

  bool Initialize(
      const std::string& json);

  bool ShouldClassifyPages() const;

  std::string ClassifyPage(
      const std::string& url,
      const std::string& content);

  CategoryList GetWinningCategories() const;

  const PageProbabilitiesCacheMap& get_page_probabilities_cache() const;

 private:
  const AdsImpl* const ads_;  // NOT OWNED

  PageProbabilitiesCacheMap page_probabilities_cache_;

  bool ShouldClassifyPagesForLocale(
      const std::string& locale) const;

  const std::string& GetPageClassification(
      const PageProbabilitiesMap& page_probabilities) const;

  CategoryProbabilitiesMap GetCategoryProbabilities(
      const PageProbabilitiesList& page_probabilities) const;

  CategoryProbabilitiesList GetWinningCategoryProbabilities(
      const CategoryProbabilitiesMap& category_probabilities,
      const int count) const;

  void CachePageProbabilities(
      const std::string& url,
      const PageProbabilitiesMap& page_probabilities);

  CategoryList ToCategoryList(
      const CategoryProbabilitiesList category_probabilities) const;

  std::unique_ptr<usermodel::UserModel> user_model_;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_PAGE_CLASSIFIER_PAGE_CLASSIFIER_H_
