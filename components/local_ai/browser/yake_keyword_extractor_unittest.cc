/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/local_ai/browser/yake_keyword_extractor.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace local_ai {

class YakeKeywordExtractorTest : public testing::Test {
 public:
  YakeKeywordExtractorTest() = default;
  ~YakeKeywordExtractorTest() override = default;

 protected:
  YakeKeywordExtractor extractor_;
};

// Test basic keyword extraction functionality
TEST_F(YakeKeywordExtractorTest, BasicKeywordExtraction) {
  std::string text =
      "Machine learning and artificial intelligence are transforming the "
      "technology industry with innovative solutions";

  auto keywords = extractor_.ExtractKeywords(text, 5, 2);

  EXPECT_FALSE(keywords.empty());
  EXPECT_LE(keywords.size(), 5u);

  // Check that all keywords have valid scores and non-empty content
  for (const auto& keyword : keywords) {
    EXPECT_FALSE(keyword.keyword.empty());
    EXPECT_GT(keyword.score, 0.0);
    EXPECT_LT(keyword.score, 1000.0);  // Reasonable upper bound for YAKE scores
  }

  // Keywords should be sorted by score (lower is better in YAKE)
  for (size_t i = 1; i < keywords.size(); ++i) {
    EXPECT_LE(keywords[i - 1].score, keywords[i].score);
  }
}

// Test with empty and invalid input
TEST_F(YakeKeywordExtractorTest, EmptyAndInvalidInput) {
  // Empty text
  auto empty_keywords = extractor_.ExtractKeywords("", 5, 2);
  EXPECT_TRUE(empty_keywords.empty());

  // Only whitespace
  auto whitespace_keywords = extractor_.ExtractKeywords("   \n\t  ", 5, 2);
  EXPECT_TRUE(whitespace_keywords.empty());

  // Only stop words
  auto stopword_keywords =
      extractor_.ExtractKeywords("the and but for with", 5, 2);
  EXPECT_TRUE(stopword_keywords.empty());

  // Only numbers
  auto number_keywords = extractor_.ExtractKeywords("123 456 789", 5, 2);
  EXPECT_TRUE(number_keywords.empty());
}

// Test filtering of numbers and meaningless fragments
TEST_F(YakeKeywordExtractorTest, NumberAndFragmentFiltering) {
  std::string text_with_numbers =
      "Travel destinations include Rome with 123 attractions and Paris with "
      "456 museums and Barcelona";

  auto keywords = extractor_.ExtractKeywords(text_with_numbers, 10, 2);

  // Should extract meaningful words, not numbers
  std::vector<std::string> keyword_strings;
  for (const auto& kw : keywords) {
    keyword_strings.push_back(kw.keyword);
  }

  // Should contain meaningful travel-related terms
  EXPECT_TRUE(std::find_if(keyword_strings.begin(), keyword_strings.end(),
                           [](const std::string& s) {
                             return s.find("Travel") != std::string::npos ||
                                    s.find("destinations") !=
                                        std::string::npos ||
                                    s.find("Rome") != std::string::npos ||
                                    s.find("Paris") != std::string::npos ||
                                    s.find("Barcelona") != std::string::npos;
                           }) != keyword_strings.end());

  // Should NOT contain pure numbers
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "123") == keyword_strings.end());
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "456") == keyword_strings.end());
}

// Test N-gram extraction (single words and phrases)
TEST_F(YakeKeywordExtractorTest, NGramExtraction) {
  std::string text =
      "Machine learning algorithms are used in artificial intelligence "
      "applications";

  // Test with max n-gram size of 2
  auto keywords = extractor_.ExtractKeywords(text, 8, 2);

  std::vector<std::string> keyword_strings;
  for (const auto& kw : keywords) {
    keyword_strings.push_back(kw.keyword);
  }

  // Should contain both single words and phrases
  bool has_single_word = false;
  bool has_phrase = false;

  for (const auto& kw : keyword_strings) {
    if (kw.find(' ') == std::string::npos) {
      has_single_word = true;
    } else {
      has_phrase = true;
    }
  }

  EXPECT_TRUE(has_single_word);  // Should have single word keywords
  EXPECT_TRUE(has_phrase);       // Should have phrase keywords
}

// Test max keywords parameter
TEST_F(YakeKeywordExtractorTest, MaxKeywordsParameter) {
  std::string long_text =
      "Technology companies are developing artificial intelligence machine "
      "learning deep learning "
      "natural language processing computer vision robotics automation "
      "algorithms neural networks "
      "data science big data analytics cloud computing distributed systems";

  // Test different max keyword limits
  auto keywords_3 = extractor_.ExtractKeywords(long_text, 3, 2);
  auto keywords_7 = extractor_.ExtractKeywords(long_text, 7, 2);
  auto keywords_15 = extractor_.ExtractKeywords(long_text, 15, 2);

  EXPECT_LE(keywords_3.size(), 3u);
  EXPECT_LE(keywords_7.size(), 7u);
  EXPECT_LE(keywords_15.size(), 15u);

  EXPECT_LE(keywords_3.size(), keywords_7.size());
  EXPECT_LE(keywords_7.size(), keywords_15.size());
}

// Test with real-world content similar to web pages
TEST_F(YakeKeywordExtractorTest, RealWorldContent) {
  std::string web_content =
      "Best travel destinations in Europe include Italy with beautiful cities "
      "like Rome and Venice. "
      "France offers cultural experiences in Paris and Lyon. Spain features "
      "Barcelona and Madrid with "
      "rich history, excellent cuisine, and unforgettable experiences for "
      "travelers seeking adventure.";

  auto keywords = extractor_.ExtractKeywords(web_content, 8, 2);

  EXPECT_FALSE(keywords.empty());

  std::vector<std::string> keyword_strings;
  for (const auto& kw : keywords) {
    keyword_strings.push_back(kw.keyword);
  }

  // Should extract travel-related terms
  int travel_related_count = 0;
  std::vector<std::string> expected_terms = {
      "travel", "destinations", "Europe",      "Italy",   "Rome",
      "Venice", "France",       "Paris",       "Spain",   "Barcelona",
      "Madrid", "cultural",     "experiences", "cuisine", "travelers"};

  for (const auto& keyword : keyword_strings) {
    for (const auto& expected : expected_terms) {
      if (keyword.find(expected) != std::string::npos ||
          expected.find(keyword) != std::string::npos) {
        travel_related_count++;
        break;
      }
    }
  }

  // Should extract meaningful travel-related keywords
  EXPECT_GT(travel_related_count, 0);
}

// Test case sensitivity handling
TEST_F(YakeKeywordExtractorTest, CaseSensitivity) {
  std::string text =
      "JavaScript Programming Language and PYTHON scripting are popular "
      "programming languages";

  auto keywords = extractor_.ExtractKeywords(text, 6, 2);

  EXPECT_FALSE(keywords.empty());

  // Should handle mixed case appropriately
  bool found_programming_related = false;
  for (const auto& kw : keywords) {
    std::string lower_kw = kw.keyword;
    std::transform(lower_kw.begin(), lower_kw.end(), lower_kw.begin(),
                   ::tolower);
    if (lower_kw.find("javascript") != std::string::npos ||
        lower_kw.find("python") != std::string::npos ||
        lower_kw.find("programming") != std::string::npos ||
        lower_kw.find("language") != std::string::npos) {
      found_programming_related = true;
      break;
    }
  }

  EXPECT_TRUE(found_programming_related);
}

// Test with content containing HTML-like fragments (should be filtered)
TEST_F(YakeKeywordExtractorTest, HTMLContentFiltering) {
  std::string html_like_content =
      "Visit our website for travel information 20px margin 15rem padding "
      "and discover amazing destinations across Europe including Italy France "
      "Spain";

  auto keywords = extractor_.ExtractKeywords(html_like_content, 8, 2);

  std::vector<std::string> keyword_strings;
  for (const auto& kw : keywords) {
    keyword_strings.push_back(kw.keyword);
  }

  // Should NOT contain CSS-like fragments
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "20px") == keyword_strings.end());
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "15rem") == keyword_strings.end());

  // Should contain meaningful content words
  bool has_travel_content = false;
  for (const auto& kw : keyword_strings) {
    if (kw.find("travel") != std::string::npos ||
        kw.find("destinations") != std::string::npos ||
        kw.find("Europe") != std::string::npos ||
        kw.find("Italy") != std::string::npos ||
        kw.find("France") != std::string::npos ||
        kw.find("Spain") != std::string::npos) {
      has_travel_content = true;
      break;
    }
  }

  EXPECT_TRUE(has_travel_content);
}

// Test filtering of navigation and UI terms from web content including "easy"
TEST_F(YakeKeywordExtractorTest, NavigationWordFiltering) {
  // Simulate actual problematic web content like the user reported
  std::string problematic_content =
      "Skip to main content Easy recipes for beginners "
      "Chicken casserole recipes Good Food cooking instructions "
      "Easy meal preparation simple guide tutorial steps";

  auto keywords = extractor_.ExtractKeywords(problematic_content, 5, 2);

  // Print results to verify against Python reference
  std::cout << "\n=== Our C++ YAKE Results ===" << std::endl;
  std::cout << "Keywords extracted:" << std::endl;
  for (const auto& kw : keywords) {
    std::cout << "  '" << kw.keyword << "' (score: " << std::fixed
              << std::setprecision(6) << kw.score << ")" << std::endl;
  }

  std::vector<std::string> keyword_strings;
  for (const auto& kw : keywords) {
    keyword_strings.push_back(kw.keyword);
  }

  // Should NOT contain navigation/UI terms that were problematic before
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "Skip") == keyword_strings.end());
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "skip") == keyword_strings.end());
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "Easy") == keyword_strings.end());
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "easy") == keyword_strings.end());
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "simple") == keyword_strings.end());
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "guide") == keyword_strings.end());
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "tutorial") == keyword_strings.end());
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(),
                        "steps") == keyword_strings.end());

  // Should contain meaningful cooking-related content
  bool has_meaningful_content = false;
  for (const auto& kw : keyword_strings) {
    if (kw.find("Chicken") != std::string::npos ||
        kw.find("chicken") != std::string::npos ||
        kw.find("casserole") != std::string::npos ||
        kw.find("recipes") != std::string::npos ||
        kw.find("cooking") != std::string::npos ||
        kw.find("Food") != std::string::npos ||
        kw.find("meal") != std::string::npos ||
        kw.find("preparation") != std::string::npos ||
        kw.find("instructions") != std::string::npos) {
      has_meaningful_content = true;
      break;
    }
  }

  EXPECT_TRUE(has_meaningful_content);
}

// Test minimum token length filtering
TEST_F(YakeKeywordExtractorTest, MinimumTokenLength) {
  std::string text =
      "AI ML is used in NLP for big data processing and machine learning "
      "applications";

  auto keywords = extractor_.ExtractKeywords(text, 6, 2);

  std::vector<std::string> keyword_strings;
  for (const auto& kw : keywords) {
    keyword_strings.push_back(kw.keyword);
  }

  // Should NOT contain very short tokens
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(), "AI") ==
              keyword_strings.end());
  EXPECT_TRUE(std::find(keyword_strings.begin(), keyword_strings.end(), "ML") ==
              keyword_strings.end());

  // Should contain longer meaningful terms
  bool has_meaningful_terms = false;
  for (const auto& kw : keyword_strings) {
    if (kw.find("machine") != std::string::npos ||
        kw.find("learning") != std::string::npos ||
        kw.find("processing") != std::string::npos ||
        kw.find("applications") != std::string::npos ||
        kw.find("data") != std::string::npos) {
      has_meaningful_terms = true;
      break;
    }
  }

  EXPECT_TRUE(has_meaningful_terms);
}

// Test score ordering (YAKE scores should be in ascending order - lower is
// better)
TEST_F(YakeKeywordExtractorTest, ScoreOrdering) {
  std::string text =
      "Artificial intelligence and machine learning technologies are "
      "revolutionizing "
      "modern software development with advanced algorithms and neural network "
      "architectures";

  auto keywords = extractor_.ExtractKeywords(text, 8, 2);

  EXPECT_GE(keywords.size(), 2u);

  // Verify scores are in ascending order (lower scores = better keywords)
  for (size_t i = 1; i < keywords.size(); ++i) {
    EXPECT_LE(keywords[i - 1].score, keywords[i].score)
        << "Keywords should be ordered by score (ascending). "
        << "Keyword " << i - 1 << " ('" << keywords[i - 1].keyword
        << "') score: " << keywords[i - 1].score << ", Keyword " << i << " ('"
        << keywords[i].keyword << "') score: " << keywords[i].score;
  }
}

}  // namespace local_ai
