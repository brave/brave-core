// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/history_embeddings/brave_page_content_extraction_service.h"
#include "brave/browser/ui/tabs/public/brave_tab_features.h"
#include "chrome/browser/page_content_annotations/page_content_extraction_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/page_content_annotations/content/page_content_extraction_service.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace {

class TestExtractionObserver
    : public page_content_annotations::PageContentExtractionService::Observer {
 public:
  void OnPageContentExtracted(
      content::Page& page,
      scoped_refptr<
          const page_content_annotations::RefCountedAnnotatedPageContent>
          page_content) override {
    page_content_ = std::move(page_content);
  }

  const page_content_annotations::RefCountedAnnotatedPageContent* page_content()
      const {
    return page_content_.get();
  }

  void Reset() { page_content_ = nullptr; }

 private:
  scoped_refptr<const page_content_annotations::RefCountedAnnotatedPageContent>
      page_content_;
};

}  // namespace

class BravePageContentExtractionTabHelperBrowserTest
    : public InProcessBrowserTest {
 public:
  BravePageContentExtractionTabHelperBrowserTest() {
    feature_list_.InitAndEnableFeature(history_embeddings::kHistoryEmbeddings);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());

    auto* service = static_cast<BravePageContentExtractionService*>(
        page_content_annotations::PageContentExtractionServiceFactory::
            GetForProfile(browser()->profile()));
    ASSERT_TRUE(service);
    service->AddObserver(&observer_);
  }

  void TearDownOnMainThread() override {
    auto* service = static_cast<BravePageContentExtractionService*>(
        page_content_annotations::PageContentExtractionServiceFactory::
            GetForProfile(browser()->profile()));
    if (service) {
      service->RemoveObserver(&observer_);
    }
    InProcessBrowserTest::TearDownOnMainThread();
  }

  BravePageContentExtractionTabHelper* GetTabHelper() {
    auto* tab = browser()->GetActiveTabInterface();
    EXPECT_TRUE(tab);
    auto* brave_features =
        tabs::BraveTabFeatures::FromTabFeatures(tab->GetTabFeatures());
    EXPECT_TRUE(brave_features);
    return brave_features->page_content_extraction_tab_helper_for_testing();
  }

 protected:
  TestExtractionObserver observer_;

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Verify the tab helper is created for each tab when kHistoryEmbeddings
// is enabled, and exercises the full extraction pipeline with a real
// renderer.
IN_PROC_BROWSER_TEST_F(BravePageContentExtractionTabHelperBrowserTest,
                       ExtractsContentOnPageLoad) {
  ASSERT_TRUE(GetTabHelper());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/simple.html")));

  ASSERT_TRUE(base::test::RunUntil(
      [this]() { return observer_.page_content() != nullptr; }));

  const auto& proto = observer_.page_content()->data;
  EXPECT_TRUE(proto.has_root_node());
  EXPECT_GT(proto.root_node().children_nodes_size(), 0);
}

IN_PROC_BROWSER_TEST_F(BravePageContentExtractionTabHelperBrowserTest,
                       SkipsNonHttpPages) {
  ASSERT_TRUE(GetTabHelper());

  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://version")));

  // Give it a moment — extraction should NOT fire for chrome:// URLs.
  base::RunLoop run_loop;
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), base::Milliseconds(500));
  run_loop.Run();

  EXPECT_EQ(observer_.page_content(), nullptr);
}

IN_PROC_BROWSER_TEST_F(BravePageContentExtractionTabHelperBrowserTest,
                       NewNavigationSupersedesOldExtraction) {
  ASSERT_TRUE(GetTabHelper());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/simple.html")));

  ASSERT_TRUE(base::test::RunUntil(
      [this]() { return observer_.page_content() != nullptr; }));

  // Reset and navigate to a different page.
  observer_.Reset();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/title1.html")));

  ASSERT_TRUE(base::test::RunUntil(
      [this]() { return observer_.page_content() != nullptr; }));

  const auto& proto = observer_.page_content()->data;
  EXPECT_TRUE(proto.has_root_node());
}
