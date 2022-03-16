/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace de_amp {

/** Test helpers */
void CheckFindCanonicalLinkResult(const std::string& expected_link,
                                  const std::string& body,
                                  const bool expected_detect_amp) {
  std::string canonical_url;
  const bool actual_detect_amp = MaybeFindCanonicalAmpUrl(body, &canonical_url);
  EXPECT_EQ(expected_detect_amp, actual_detect_amp);
  if (expected_detect_amp) {
    EXPECT_EQ(expected_link, canonical_url);
  }
}
void CheckCheckCanonicalLinkResult(const std::string& canonical_link,
                                   const std::string& original,
                                   const bool expected) {
  GURL canonical_url(canonical_link), original_url(original);
  EXPECT_EQ(expected, VerifyCanonicalAmpUrl(canonical_url, original_url));
}

/** De AMP Util Tests */
TEST(DeAmpUtilUnitTest, DetectAmpWithEmoji) {
  const std::string body =
      "<html ⚡>"
      "<head>"
      "<link rel=\"canonical\" href=\"https://abc.com\"/>"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true);
}

TEST(DeAmpUtilUnitTest, DetectAmpWithWordAmp) {
  const std::string body =
      "<html amp>"
      "<head>"
      "<link rel=\"author\" href=\"https://xyz.com\"/>"
      "<link rel=\"canonical\" href=\"https://abc.com\"/>"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true);
}

TEST(DeAmpUtilUnitTest, DetectAmpWithWordAmpNotAtEnd) {
  const std::string body =
      "<html amp xyzzy>"
      "<head>"
      "<link rel=\"author\" href=\"https://xyz.com\"/>"
      "<link rel=\"canonical\" href=\"https://abc.com\"/>"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true);
}

TEST(DeAmpUtilUnitTest, DetectAmpMixedCase) {
  const std::string body =
      "<DOCTYPE! html>\n"
      "<html AmP xyzzy>\n"
      "<head>\n"
      "<link rel=\"author\" href=\"https://xyz.com\"/>\n"
      "<link rel=\"canonical\" "
      "href=\"https://abc.com\"/></head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true);
}

TEST(DeAmpUtilUnitTest, NegativeDetectAmp) {
  // Put AMP attribute in a different tag than html
  const std::string body =
      "<html xyzzy>\n"
      "<head>\n"
      "<link amp rel=\"author\" href=\"https://xyz.com\"/>\n"
      "<link rel=\"canonical\" href=\"https://abc.com\"/>\n"
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
      "<link amp rel=\"author\" href=\"https://xyz.com\"/>\n"
      "<link rel=\"canonical\" href=\"https://abc.com\"/>"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("", body, false);
}

TEST(DeAmpUtilUnitTest, MalformedHtmlDoc) {
  const std::string body =
      "<xyz html amp xyzzy>\n"
      "<head>"
      "<link amp rel=\"author\" href=\"https://xyz.com\"/>\n"
      "<link rel=\"canonical\" href=\"https://abc.com\"/>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("", body, false);
}

TEST(DeAmpUtilUnitTest, LinkRelNotInSameTag) {
  // Checking to make sure a random "canonical" does not confused parser
  const std::string body =
      "<html amp>\n"
      "<head>"
      "<link rel=\"author\" href=\"https://xyz.com\"/>\n"
      "<body>"
      "\"canonical\"> href=\"https://abc.com\"/>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("", body, false);
}

TEST(DeAmpUtilUnitTest, SingleQuotes) {
  const std::string body =
      "<DOCTYPE! html>"
      "<html AMP xyzzy>\n"
      "<head><link rel='author' href='https://xyz.com'/>\n"
      "<link rel='canonical' href='https://abc.com'>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true);
}

TEST(DeAmpUtilUnitTest, CanonicalLinkMissingScheme) {
  CheckCheckCanonicalLinkResult("xyz.com", "https://amp.xyz.com", false);
}

TEST(DeAmpUtilUnitTest, HttpsCanonicalLinkCorrect) {
  CheckCheckCanonicalLinkResult("https://xyz.com", "https://amp.xyz.com", true);
}

TEST(DeAmpUtilUnitTest, HttpCanonicalLinkCorrect) {
  CheckCheckCanonicalLinkResult("http://xyz.com", "http://amp.xyz.com", true);
}

TEST(DeAmpUtilUnitTest, CanonicalLinkSameAsOriginal) {
  CheckCheckCanonicalLinkResult("https://amp.xyz.com", "https://amp.xyz.com",
                                false);
}

TEST(DeAmpUtilUnitTest, CanonicalLinkNotHttpOrHttps) {
  CheckCheckCanonicalLinkResult("ftp://xyz.com", "https://amp.xyz.com", false);
}

TEST(DeAmpUtilUnitTest, CanonicalLinkIsRelative) {
  CheckCheckCanonicalLinkResult("abc", "https://amp.xyz.com", false);
}

}  // namespace de_amp
