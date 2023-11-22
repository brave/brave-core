/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_unittest_helper.h"

#include <string>
#include <vector>

namespace brave_ads::test {

TextClassificationHelper::TextClassificationHelper() : processor_(resource_) {}

TextClassificationHelper::~TextClassificationHelper() = default;

void TextClassificationHelper::Mock() {
  const std::vector<std::string> texts = {
      "Savoring food and drinks, life's simple pleasure.",
      "Decentralization frees finance, making banking borderless.",
      "Technology and computing shape our future."};

  for (const auto& text : texts) {
    processor_.Process(text);
  }
}

// static
SegmentList TextClassificationHelper::Expectation() {
  return {"personal finance-banking",
          "technology & computing-technology & computing",
          "food & drink-cocktails",
          "home-home",
          "business-marketing",
          "food & drink-vegetarian",
          "fashion-jewelry",
          "science-geology",
          "personal finance-personal finance",
          "sports-surfing",
          "sports-baseball",
          "sports-fishing",
          "folklore-paranormal phenomena",
          "hobbies & interests-needlework",
          "science-mathematics",
          "food & drink-coffee",
          "law-law",
          "arts & entertainment-film",
          "health & fitness-bodybuilding",
          "sports-skiing",
          "food & drink-cooking",
          "family & parenting-pregnancy",
          "food & drink-cheese",
          "food & drink-tea",
          "technology & computing-programming",
          "history-archaeology",
          "other-other",
          "hobbies & interests-sci-fi",
          "arts & entertainment-radio",
          "arts & entertainment-animation",
          "arts & entertainment-poetry",
          "technology & computing-software",
          "food & drink-wine",
          "science-economics",
          "technology & computing-windows",
          "real estate-mortgages",
          "science-palaeontology",
          "arts & entertainment-anime",
          "food & drink-barbecues & grilling",
          "folklore-astrology",
          "hobbies & interests-horse racing",
          "food & drink-baking",
          "home-appliances",
          "business-business",
          "health & fitness-alternative medicine",
          "arts & entertainment-arts & entertainment",
          "sports-martial arts",
          "family & parenting-parenting",
          "personal finance-tax",
          "pets-pets",
          "sports-climbing",
          "weather-weather",
          "automotive-motorcycles",
          "science-mechanics",
          "health & fitness-diet & nutrition",
          "science-chemistry",
          "folklore-folklore",
          "education-education",
          "personal finance-investing",
          "arts & entertainment-television",
          "health & fitness-sex",
          "hobbies & interests-genealogy",
          "personal finance-insurance",
          "sports-golf",
          "fashion-clothing",
          "hobbies & interests-hobbies & interests",
          "hobbies & interests-dance",
          "science-biology",
          "travel-travel",
          "food & drink-vegan",
          "sports-snowboarding",
          "technology & computing-unix",
          "education-homeschooling",
          "technology & computing-apple",
          "hobbies & interests-board games",
          "sports-diving",
          "military-military",
          "health & fitness-exercise",
          "careers-careers",
          "pets-birds",
          "hobbies & interests-smoking",
          "food & drink-cider",
          "sports-rugby",
          "science-science",
          "sports-bowling",
          "health & fitness-dental care",
          "sports-volleyball",
          "hobbies & interests-arts & crafts",
          "sports-jogging",
          "arts & entertainment-design",
          "hobbies & interests-gambling",
          "sports-olympics",
          "personal finance-retirement planning",
          "home-interior design",
          "gaming-gaming",
          "sports-cricket",
          "sports-yoga",
          "personal finance-credit & debt & loans",
          "pets-dogs",
          "food & drink-food & drink",
          "sports-sports",
          "pets-aquariums",
          "sports-hunting",
          "business-energy",
          "sports-athletics",
          "food & drink-beer",
          "hobbies & interests-coins",
          "science-astronomy",
          "automotive-pickup trucks",
          "technology & computing-freeware",
          "science-physics",
          "sports-boxing",
          "sports-tennis",
          "real estate-real estate",
          "sports-cycling",
          "pets-cats"};
}

}  // namespace brave_ads::test
