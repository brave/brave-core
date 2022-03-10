/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace de_amp {

/** Test helpers */
bool CheckIfAmpDetected(const std::string& body, std::string* canonical_link) {
  return FindCanonicalLinkIfAMP(body, canonical_link);
}
void CheckFindCanonicalLinkResult(const std::string& expected_link,
                                  const std::string& body,
                                  const bool expected_detect_amp) {
  std::string actual_link;
  const bool actual_detect_amp = CheckIfAmpDetected(body, &actual_link);
  EXPECT_EQ(expected_detect_amp, actual_detect_amp);
  if (expected_detect_amp) {
    EXPECT_EQ(expected_link, actual_link);
  }
}
void CheckCheckCanonicalLinkResult(const std::string& canonical_link,
                                   const std::string& original,
                                   const bool expected) {
  GURL canonical_url(canonical_link), original_url(original);
  EXPECT_EQ(expected, VerifyCanonicalLink(canonical_url, original_url));
}

/** De AMP Util Tests */
TEST(DeAmpUtilUnitTest, DetectAmpWithEmoji) {
  const std::string body =
      "<html ⚡>"
      "<head>"
      "<link rel=\"canonical\" href=\"abc\"/>"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("abc", body, true);
}

TEST(DeAmpUtilUnitTest, DetectAmpWithWordAmp) {
  const std::string body =
      "<html amp>"
      "<head>"
      "<link rel=\"author\" href=\"xyz\"/>"
      "<link rel=\"canonical\" href=\"abc\"/>"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("abc", body, true);
}

TEST(DeAmpUtilUnitTest, DetectAmpWithWordAmpNotAtEnd) {
  const std::string body =
      "<html amp xyzzy>"
      "<head>"
      "<link rel=\"author\" href=\"xyz\"/>"
      "<link rel=\"canonical\" href=\"abc\"/>"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("abc", body, true);
}

TEST(DeAmpUtilUnitTest, DetectAmpMixedCase) {
  const std::string body =
      "<DOCTYPE! html>\n"
      "<html AmP xyzzy>\n"
      "<head>\n"
      "<link rel=\"author\" href=\"xyz\"/>\n"
      "<link rel=\"canonical\" href=\"abc\"/></head><body></body></html>";
  CheckFindCanonicalLinkResult("abc", body, true);
}

TEST(DeAmpUtilUnitTest, NegativeDetectAmp) {
  // Put AMP attribute in a different tag than html
  const std::string body =
      "<html xyzzy>\n"
      "<head>\n"
      "<link amp rel=\"author\" href=\"xyz\"/>\n"
      "<link rel=\"canonical\" href=\"abc\"/>\n"
      "</head>\n"
      "<body></body>\n"
      "</html>";
  CheckFindCanonicalLinkResult("", body, false);
}

TEST(DeAmpUtilUnitTest, DetectAmpButNoCanonicalLink) {
  // Put AMP attribute in a different tag than html
  const std::string body =
      "<html xyzzy>"
      "<head>"
      "<link amp rel=\"author\" href=\"xyz\"/>\n"
      "<link rel=\"canonical\" href=\"abc\"/>"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("", body, false);
}

TEST(DeAmpUtilUnitTest, MalformedHtmlDoc) {
  const std::string body =
      "<xyz html amp xyzzy>\n"
      "<head>"
      "<link amp rel=\"author\" href=\"xyz\"/>\n"
      "<link rel=\"canonical\" href=\"abc\"/>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("", body, false);
}

TEST(DeAmpUtilUnitTest, LinkRelNotInSameTag) {
  // Checking to make sure a random "canonical" does not confused parser
  const std::string body =
      "<html amp>\n"
      "<head>"
      "<link rel=\"author\" href=\"xyz\"/>\n"
      "<body>"
      "\"canonical\"> href=\"abc\"/>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("", body, false);
}

TEST(DeAmpUtilUnitTest, SingleQuotes) {
  const std::string body =
      "<DOCTYPE! html>"
      "<html AMP xyzzy>\n"
      "<head><link rel='author' href='xyz'/>\n"
      "<link rel='canonical' href='abc'>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("abc", body, true);
}

TEST(DeAmpUtilUnitTest, CanonicalLinkMalformed) {
  CheckCheckCanonicalLinkResult("xyz.com", "https://amp.xyz.com", false);
}

TEST(DeAmpUtilUnitTest, CanonicalLinkCorrect) {
  CheckCheckCanonicalLinkResult("https://xyz.com", "https://amp.xyz.com", true);
}

TEST(DeAmpUtilUnitTest, CanonicalLinkSameAsOriginal) {
  CheckCheckCanonicalLinkResult("https://amp.xyz.com", "https://amp.xyz.com",
                                false);
}

TEST(DeAmpUtilUnitTest, CanonicalLinkNotHttp) {
  CheckCheckCanonicalLinkResult("ftp://xyz.com", "https://amp.xyz.com", false);
}

}  // namespace de_amp
