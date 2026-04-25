/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_url_pattern_constants.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSearchEngineUrlPatternConstantsTest, GetAmazonUrlPattern) {
  // Act & Assert
  EXPECT_EQ(
      "https://"
      "www.amazon.(ae|ca|cn|co.jp|co.uk|com|com.au|com.br|com.mx|de|eg|es|fr|"
      "in|it|nl|pl|sa|se|sp|tr)/",
      GetAmazonUrlPattern());
}

TEST(BraveAdsSearchEngineUrlPatternConstantsTest, GetGoogleUrlPattern) {
  // Act & Assert
  EXPECT_EQ(
      "https://"
      "www.google.(ac|ad|ae|al|am|as|at|az|ba|be|bf|bg|bi|bj|bs|bt|ca|cat|cd|"
      "cf|cg|ch|ci|cl|cm|cn|co.bw|co.ck|co.cr|co.id|co.il|co.im|co.in|co.je|co."
      "jp|co.ke|co.kr|co.ls|co.ma|co.mz|co.nz|co.th|co.tz|co.ug|co.uk|co.uz|co."
      "ve|co.vi|co.za|co.zm|co.zw|com|com.af|com.ag|com.ai|com.ar|com.au|com."
      "bd|com.bh|com.bn|com.bo|com.br|com.by|com.bz|com.co|com.cu|com.cy|com."
      "do|com.ec|com.eg|com.et|com.fj|com.gh|com.gi|com.gt|com.hk|com.jm|com."
      "kg|com.kh|com.kw|com.lb|com.ly|com.mt|com.mx|com.my|com.na|com.nf|com."
      "ng|com.ni|com.np|com.om|com.pa|com.pe|com.pg|com.ph|com.pk|com.pr|com."
      "py|com.qa|com.sa|com.sb|com.sg|com.sl|com.sv|com.tj|com.tr|com.tw|com."
      "ua|com.uy|com.vc|com.vn|cv|cz|de|dj|dk|dm|dz|ee|es|fi|fm|fr|ga|ge|gg|gl|"
      "gm|gp|gr|gy|hn|hr|ht|hu|ie|iq|is|it|it.ao|jo|ki|kz|la|li|lk|lt|lu|lv|md|"
      "me|mg|mk|ml|mn|ms|mu|mv|mw|ne|nl|no|nr|nu|pl|pn|ps|pt|ro|rs|ru|rw|sc|se|"
      "sh|si|sk|sm|sn|so|sr|st|td|tg|tk|tl|tm|tn|to|tt|vg|vu|ws)/",
      GetGoogleUrlPattern());
}

TEST(BraveAdsSearchEngineUrlPatternConstantsTest, GetMojeekUrlPattern) {
  // Act & Assert
  EXPECT_EQ("https://www.mojeek.(co.uk|com)/", GetMojeekUrlPattern());
}

TEST(BraveAdsSearchEngineUrlPatternConstantsTest, GetWikipediaUrlPattern) {
  // Act & Assert
  EXPECT_EQ(
      "https://"
      "(af|ar|arz|ast|az|azb|be|bg|bn|ca|ce|ceb|cs|cy|da|de|el|en|eo|es|et|eu|"
      "fa|fi|fr|gl|he|hi|hr|hu|hy|id|it|ja|ka|kk|ko|la|lt|lv|min|mk|ms|my|nan|"
      "nl|nn|no|pl|pt|ro|ru|sh|simple|sk|sl|sr|sv|ta|tg|th|tr|tt|uk|ur|uz|vi|"
      "vo|war|zh|zh-yue).wikipedia.org/",
      GetWikipediaUrlPattern());
}

TEST(BraveAdsSearchEngineUrlPatternConstantsTest, GetYahooUrlPattern) {
  // Act & Assert
  EXPECT_EQ(
      "https://"
      "((au|be|br|ca|de|en-maktoob|es|espanol|fr|fr-be|gr|hk|id|ie|in|it|"
      "malaysia|nz|ph|qc|ro|se|sg|tw|uk|vn|www|za).search.yahoo.com/"
      "|search.yahoo.com/)",
      GetYahooUrlPattern());
}

}  // namespace brave_ads
