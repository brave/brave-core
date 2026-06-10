/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/search_engines/template_url_unittest.cc>

// Brave replaces the upstream URLRefTestSearchSource test, which expects
// {google:searchSource} to expand to source=chrome.ob / source=chrome.rb. Our
// chromium_src override of template_url.cc suppresses that expansion because
// the param is Chrome-specific attribution that Brave does not want to send to
// Google.

TEST_F(TemplateURLTest, NoSourceParamFromOmnibox) {
  TestingSearchTermsData search_terms_data("http://www.google.com/");
  TemplateURLData data;
  data.SetURL("http://foo/?q={searchTerms}&{google:searchSource}");
  TemplateURL url(data);
  ASSERT_TRUE(url.url_ref().IsValid(search_terms_data));
  ASSERT_TRUE(url.url_ref().SupportsReplacement(search_terms_data));

  TemplateURLRef::SearchTermsArgs args(u"X");
  args.page_classification = metrics::OmniboxEventProto::NTP;
  GURL result(url.url_ref().ReplaceSearchTerms(args, search_terms_data));
  EXPECT_EQ("http://foo/?q=X&", result.spec());
}

TEST_F(TemplateURLTest, NoSourceParamFromRealbox) {
  TestingSearchTermsData search_terms_data("http://www.google.com/");
  TemplateURLData data;
  data.SetURL("http://foo/?q={searchTerms}&{google:searchSource}");
  TemplateURL url(data);

  TemplateURLRef::SearchTermsArgs args(u"X");
  args.page_classification = metrics::OmniboxEventProto::NTP_REALBOX;
  GURL result(url.url_ref().ReplaceSearchTerms(args, search_terms_data));
  EXPECT_EQ("http://foo/?q=X&", result.spec());
}

TEST_F(TemplateURLTest, NoSourceParamFromOtherClassification) {
  TestingSearchTermsData search_terms_data("http://www.google.com/");
  TemplateURLData data;
  data.SetURL("http://foo/?q={searchTerms}&{google:searchSource}");
  TemplateURL url(data);

  TemplateURLRef::SearchTermsArgs args(u"X");
  args.page_classification = metrics::OmniboxEventProto::ANDROID_HUB;
  GURL result(url.url_ref().ReplaceSearchTerms(args, search_terms_data));
  EXPECT_EQ("http://foo/?q=X&", result.spec());
}

// Sanity check: Brave Search's URL template hard-codes &source=desktop or
// &source=android and does not use {google:searchSource}, so the param it
// embeds must survive ReplaceSearchTerms intact.
TEST_F(TemplateURLTest, BraveSearchSourceParamPreserved) {
  TestingSearchTermsData search_terms_data("http://www.google.com/");
  TemplateURLData data;
  data.SetURL("https://search.brave.com/search?q={searchTerms}&source=desktop");
  TemplateURL url(data);

  TemplateURLRef::SearchTermsArgs args(u"X");
  args.page_classification = metrics::OmniboxEventProto::NTP;
  GURL result(url.url_ref().ReplaceSearchTerms(args, search_terms_data));
  EXPECT_EQ("https://search.brave.com/search?q=X&source=desktop",
            result.spec());
}
