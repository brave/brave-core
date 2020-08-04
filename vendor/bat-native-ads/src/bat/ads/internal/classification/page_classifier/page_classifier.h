/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLASSIFICATION_PAGE_CLASSIFIER_PAGE_CLASSIFIER_H_
#define BAT_ADS_INTERNAL_CLASSIFICATION_PAGE_CLASSIFIER_PAGE_CLASSIFIER_H_

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bat/ads/result.h"
#include "bat/usermodel/user_model.h"

namespace ads {

class AdsImpl;

namespace classification {

using PageProbabilitiesMap = std::map<std::string, double>;
using PageProbabilitiesList = std::deque<PageProbabilitiesMap>;
using PageProbabilitiesCacheMap = std::map<std::string, PageProbabilitiesMap>;

using CategoryProbabilityPair = std::pair<std::string, double>;
using CategoryProbabilitiesList = std::vector<CategoryProbabilityPair>;
using CategoryProbabilitiesMap = std::map<std::string, double>;

using CategoryList = std::vector<std::string>;

const char kUntargeted[] = "untargeted";

class PageClassifier {
 public:
  PageClassifier(
     AdsImpl* ads);

  ~PageClassifier();

  void LoadUserModelForLocale(
      const std::string& locale);
  void LoadUserModelForId(
      const std::string& id);

  std::string MaybeClassifyPage(
      const std::string& url,
      const std::string& content);

  CategoryList GetWinningCategories() const;

  const PageProbabilitiesCacheMap& get_page_probabilities_cache() const;

 private:
  AdsImpl* ads_;  // NOT OWNED

  PageProbabilitiesCacheMap page_probabilities_cache_;

  bool IsInitialized() const;

  bool Initialize(
      const std::string& json);

  void OnLoadUserModelForId(
      const std::string& id,
      const Result result,
      const std::string& json);

  bool ShouldClassifyPages() const;

  std::string ClassifyPage(
      const std::string& url,
      const std::string& content);

  std::string GetPageClassification(
      const PageProbabilitiesMap& page_probabilities) const;

  bool ShouldFilterCategory(
      const std::string& category) const;

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

}  // namespace classification
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLASSIFICATION_PAGE_CLASSIFIER_PAGE_CLASSIFIER_H_
