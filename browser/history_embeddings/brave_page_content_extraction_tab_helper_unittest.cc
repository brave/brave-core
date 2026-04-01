// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/history_embeddings/brave_page_content_extraction_tab_helper.h"

#include <memory>

#include "brave/browser/history_embeddings/brave_page_content_extraction_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/optimization_guide/proto/features/common_quality_data.pb.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/page_content_annotations/content/page_content_extraction_service.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "testing/gtest/include/gtest/gtest.h"

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
    call_count_++;
  }

  const page_content_annotations::RefCountedAnnotatedPageContent* page_content()
      const {
    return page_content_.get();
  }

  int call_count() const { return call_count_; }

 private:
  scoped_refptr<const page_content_annotations::RefCountedAnnotatedPageContent>
      page_content_;
  int call_count_ = 0;
};

}  // namespace

class BravePageContentExtractionTabHelperTest
    : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    extraction_service_ = std::make_unique<BravePageContentExtractionService>(
        g_browser_process->os_crypt_async(), profile()->GetPath(),
        /*tracker=*/nullptr);
    extraction_service_->AddObserver(&observer_);

    tab_interface_ = std::make_unique<tabs::MockTabInterface>();
    ON_CALL(*tab_interface_, GetContents())
        .WillByDefault(testing::Return(web_contents()));

    helper_ = std::make_unique<BravePageContentExtractionTabHelper>(
        *tab_interface_, extraction_service_.get());
  }

  void TearDown() override {
    helper_.reset();
    tab_interface_.reset();
    extraction_service_->RemoveObserver(&observer_);
    extraction_service_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

 protected:
  TestExtractionObserver observer_;
  std::unique_ptr<BravePageContentExtractionService> extraction_service_;
  std::unique_ptr<tabs::MockTabInterface> tab_interface_;
  std::unique_ptr<BravePageContentExtractionTabHelper> helper_;
};

TEST_F(BravePageContentExtractionTabHelperTest, SkipsNonHttpUrls) {
  NavigateAndCommit(GURL("chrome://version"));
  // Simulate the WebContentsObserver callback via the public interface.
  static_cast<content::WebContentsObserver*>(helper_.get())->DidStopLoading();
  // No mojo call should be attempted for chrome:// URLs.
  // The observer should not be notified.
  EXPECT_EQ(observer_.call_count(), 0);
}

TEST_F(BravePageContentExtractionTabHelperTest,
       ServiceObserverReceivesContent) {
  NavigateAndCommit(GURL("https://example.com"));

  auto apc = base::MakeRefCounted<
      page_content_annotations::RefCountedAnnotatedPageContent>();
  auto* root = apc->data.mutable_root_node();
  auto* child = root->add_children_nodes();
  auto* attrs = child->mutable_content_attributes();
  attrs->set_attribute_type(optimization_guide::proto::CONTENT_ATTRIBUTE_TEXT);
  attrs->mutable_text_data()->set_text_content("Hello world");
  apc->data.mutable_main_frame_data()->set_title("Test Page");
  apc->data.mutable_main_frame_data()->set_url("https://example.com");

  extraction_service_->NotifyPageContentExtracted(
      web_contents()->GetPrimaryPage(), std::move(apc));

  ASSERT_EQ(observer_.call_count(), 1);
  ASSERT_NE(observer_.page_content(), nullptr);

  const auto& proto = observer_.page_content()->data;
  EXPECT_TRUE(proto.has_root_node());
  ASSERT_GT(proto.root_node().children_nodes_size(), 0);

  // Verify the text content is preserved through the pipeline.
  const auto& text_node = proto.root_node().children_nodes(0);
  EXPECT_EQ(text_node.content_attributes().text_data().text_content(),
            "Hello world");
  EXPECT_EQ(proto.main_frame_data().title(), "Test Page");
  EXPECT_EQ(proto.main_frame_data().url(), "https://example.com");
}
