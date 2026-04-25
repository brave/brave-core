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
                                  const bool expected_detect_amp,
                                  const bool expected_find_canonical) {
  const bool actual_detect_amp = CheckIfAmpPage(body);
  EXPECT_EQ(expected_detect_amp, actual_detect_amp);
  if (expected_detect_amp) {  // Only check for canonical link if this is an AMP
                              // page
    auto canonical_link = FindCanonicalAmpUrl(body);
    EXPECT_EQ(expected_find_canonical, canonical_link.has_value());
    if (expected_find_canonical) {
      EXPECT_EQ(expected_link, canonical_link.value());
    }
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
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
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
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
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
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, DetectAmpWithAmpEmptyAttribute) {
  const std::string body =
      "<html amp=\"\" xyzzy>"
      "<head>"
      "<link rel=\"canonical\" href=\"https://abc.com\"/>"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, DetectAmpWithEmojiEmptyAttribute) {
  const std::string body =
      "<html tomato ⚡=\"\" xyzzy >"
      "<head>"
      "<link rel=\"canonical\" href=\"https://abc.com\"/>"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, DetectAmpWithEmojiEmptyAttributeSingleQuotes) {
  const std::string body =
      "<html tomato ⚡='' xyzzy >"
      "<head>"
      "<link rel=\"canonical\" href=\"https://abc.com\"/>"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, DetectAmpMixedCase) {
  const std::string body =
      "<DOCTYPE! html>\n"
      "<html AmP xyzzy>\n"
      "<head>\n"
      "<link rel=\"author\" href=\"https://xyz.com\"/>\n"
      "<link rel=\"canonical\" "
      "href=\"https://abc.com\"/></head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
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
  CheckFindCanonicalLinkResult("", body, false, false);
}

TEST(DeAmpUtilUnitTest, DetectAmpButNoCanonicalLink) {
  const std::string body =
      "<html amp xyzzy>"
      "<head>"
      "<link amp rel=\"author\" href=\"https://xyz.com\"/>\n"
      "</head>"
      "<body></body>"
      "</html>";
  CheckFindCanonicalLinkResult("", body, true, false);
}

TEST(DeAmpUtilUnitTest, MalformedHtmlDoc) {
  const std::string body =
      "<xyz html amp xyzzy>\n"
      "<head>"
      "<link amp rel=\"author\" href=\"https://xyz.com\"/>\n"
      "<link rel=\"canonical\" href=\"https://abc.com\"/>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("", body, false, false);
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
  CheckFindCanonicalLinkResult("", body, true, false);
}

TEST(DeAmpUtilUnitTest, SingleQuotes) {
  const std::string body =
      "<DOCTYPE! html>"
      "<html AMP xyzzy>\n"
      "<head><link rel='author' href='https://xyz.com'/>\n"
      "<link rel='canonical' href='https://abc.com'>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, NoQuotes) {
  const std::string body =
      "<DOCTYPE! html>"
      "<html AMP xyzzy>\n"
      "<head><link rel=author href=https://xyz.com/>\n"
      "<link href=https://abc.com rel=canonical>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, NoQuotesEndingWithHref) {
  const std::string body =
      "<DOCTYPE! html>"
      "<html AMP xyzzy>\n"
      "<head><link rel=author href=https://xyz.com/>\n"
      "<link rel=canonical href=https://abc.com/>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, NoQuotesEndingWithSpaceSlashAngleBracket) {
  const std::string body =
      "<DOCTYPE! html>"
      "<html AMP xyzzy>\n"
      "<head><link rel=author href=https://xyz.com/>\n"
      "<link rel=canonical href=https://abc.com />"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, NoQuotesEndingWithAngleBracket) {
  const std::string body =
      "<DOCTYPE! html>"
      "<html AMP xyzzy>\n"
      "<head><link rel=author href=https://xyz.com/>\n"
      "<link rel=canonical href=https://abc.com>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, NoQuotesEndingWithSpaceAngleBracket) {
  const std::string body =
      "<DOCTYPE! html>"
      "<html AMP xyzzy>\n"
      "<head>\n<link rel=canonical href=https://abc.com ><link rel=author "
      "href=https://xyz.com/>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, SingleQuotesWithTrueAttribute) {
  const std::string body =
      "<DOCTYPE! html>"
      "<html AMP=\'true\'>\n"
      "<head>\n<link rel=canonical href=https://abc.com ><link rel=author "
      "href=https://xyz.com/>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, DoubleQuotesWithTrueAttribute) {
  const std::string body =
      "<DOCTYPE! html>"
      "<html AMP=\"true\">\n"
      "<head>\n<link rel=canonical href=https://abc.com ><link rel=author "
      "href=https://xyz.com/>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
}

TEST(DeAmpUtilUnitTest, DoubleQuotesWithTrueAttributeUpperCase) {
  const std::string body =
      "<DOCTYPE! html>"
      "<html AMP=\"TRUE\">\n"
      "<head>\n<link rel=canonical href=https://abc.com ><link rel=author "
      "href=https://xyz.com/>"
      "</head><body></body></html>";
  CheckFindCanonicalLinkResult("https://abc.com", body, true, true);
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
