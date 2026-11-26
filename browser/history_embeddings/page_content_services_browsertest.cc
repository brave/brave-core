// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/scoped_feature_list.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#include "chrome/browser/page_content_annotations/page_content_annotations_service_factory.h"
#include "chrome/browser/page_content_annotations/page_content_extraction_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace history_embeddings {

class PageContentServicesBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  PageContentServicesBrowserTest() {
    feature_list_.InitWithFeatureState(kHistoryEmbeddings, GetParam());
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_P(PageContentServicesBrowserTest,
                       ExtractionServiceFollowsHistoryEmbeddings) {
  EXPECT_EQ(GetParam(),
            !!page_content_annotations::PageContentExtractionServiceFactory::
                 GetForProfile(browser()->GetProfile()));
}

IN_PROC_BROWSER_TEST_P(PageContentServicesBrowserTest,
                       AnnotationsServiceFollowsHistoryEmbeddings) {
  EXPECT_EQ(GetParam(), !!PageContentAnnotationsServiceFactory::GetForProfile(
                            browser()->GetProfile()));
}

INSTANTIATE_TEST_SUITE_P(,
                         PageContentServicesBrowserTest,
                         testing::Bool(),
                         [](const testing::TestParamInfo<bool>& info) {
                           return info.param ? "HistoryEmbeddingsOn"
                                             : "HistoryEmbeddingsOff";
                         });

}  // namespace history_embeddings
