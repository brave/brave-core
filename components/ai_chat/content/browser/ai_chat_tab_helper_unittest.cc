// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"

#include <memory>
#include <vector>

#include "base/callback_list.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/components/ai_chat/content/browser/associated_web_contents_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

class FakePrintPreviewExtractor
    : public AssociatedWebContentsContent::PrintPreviewExtractionDelegate {
 public:
  void CaptureImages(CaptureImagesCallback callback) override {}
};

// Records, when the observed content is destroyed, which content the tab helper
// serves at that point.
class DestroyedObserver : public AssociatedContentDelegate::Observer {
 public:
  explicit DestroyedObserver(AIChatTabHelper& tab_helper)
      : tab_helper_(tab_helper) {}

  void OnDestroyed(AssociatedContentDelegate* delegate) override {
    destroyed_ = true;
    destroyed_was_served_ = delegate == &tab_helper_->web_contents_content();
    served_when_destroyed_ = &tab_helper_->web_contents_content();
  }

  bool destroyed() const { return destroyed_; }
  bool destroyed_was_served() const { return destroyed_was_served_; }
  AssociatedWebContentsContent* served_when_destroyed() const {
    return served_when_destroyed_;
  }

 private:
  const raw_ref<AIChatTabHelper> tab_helper_;
  bool destroyed_ = false;
  bool destroyed_was_served_ = false;
  raw_ptr<AssociatedWebContentsContent> served_when_destroyed_ = nullptr;
};

}  // namespace

class AIChatTabHelperUnitTest : public content::RenderViewHostTestHarness {
 public:
  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    tab_contents_ = web_contents();
    ON_CALL(tab_, GetContents()).WillByDefault([this] {
      return tab_contents_.get();
    });
    ON_CALL(tab_, RegisterWillDiscardContents)
        .WillByDefault(
            [this](tabs::TabInterface::WillDiscardContentsCallback callback) {
              return will_discard_contents_callbacks_.Add(std::move(callback));
            });
  }

  void TearDown() override {
    tab_contents_ = nullptr;
    replacement_contents_.clear();
    content::RenderViewHostTestHarness::TearDown();
  }

  // Records each contents a print preview extraction delegate is created for.
  AIChatTabHelper::PrintPreviewExtractionDelegateFactory
  RecordingDelegateFactory() {
    return base::BindRepeating(
        [](std::vector<content::WebContents*>* created_for,
           content::WebContents* web_contents)
            -> std::unique_ptr<
                AssociatedWebContentsContent::PrintPreviewExtractionDelegate> {
          created_for->push_back(web_contents);
          return std::make_unique<FakePrintPreviewExtractor>();
        },
        &delegates_created_for_);
  }

  // Replaces the tab's contents as discarding the tab does: observers are told
  // before the tab returns the new contents.
  content::WebContents* DiscardContents() {
    auto new_contents = content::WebContentsTester::CreateTestWebContents(
        browser_context(), nullptr);
    content::WebContents* const old_contents = tab_contents_;
    will_discard_contents_callbacks_.Notify(&tab_, old_contents,
                                            new_contents.get());
    tab_contents_ = new_contents.get();
    replacement_contents_.push_back(std::move(new_contents));
    return tab_contents_;
  }

 protected:
  tabs::MockTabInterface tab_;
  std::vector<content::WebContents*> delegates_created_for_;

 private:
  raw_ptr<content::WebContents> tab_contents_ = nullptr;
  std::vector<std::unique_ptr<content::WebContents>> replacement_contents_;
  base::RepeatingCallbackList<
      void(tabs::TabInterface*, content::WebContents*, content::WebContents*)>
      will_discard_contents_callbacks_;
};

TEST_F(AIChatTabHelperUnitTest, ServesTheTabContents) {
  AIChatTabHelper tab_helper(tab_, RecordingDelegateFactory());

  EXPECT_EQ(AIChatTabHelper::From(&tab_), &tab_helper);
  EXPECT_EQ(&tab_helper.GetWebContents(), web_contents());
  EXPECT_EQ(tab_helper.web_contents_content().web_contents(), web_contents());
  EXPECT_THAT(delegates_created_for_, testing::ElementsAre(web_contents()));
  EXPECT_TRUE(tab_helper.web_contents_content()
                  .GetPrintPreviewExtractionDelegateForTesting());
}

TEST_F(AIChatTabHelperUnitTest, NoDelegateWithoutFactory) {
  AIChatTabHelper tab_helper(
      tab_, AIChatTabHelper::PrintPreviewExtractionDelegateFactory());

  EXPECT_FALSE(tab_helper.web_contents_content()
                   .GetPrintPreviewExtractionDelegateForTesting());
}

// Discarding the tab keeps the tab helper, which serves a new content for the
// new contents.
TEST_F(AIChatTabHelperUnitTest, FollowsDiscardedContents) {
  AIChatTabHelper tab_helper(tab_, RecordingDelegateFactory());
  // Only compared: the old and new contents' content exist at the same time,
  // so they cannot share an address.
  const AssociatedWebContentsContent* const old_content =
      &tab_helper.web_contents_content();

  content::WebContents* const new_contents = DiscardContents();

  EXPECT_EQ(AIChatTabHelper::From(&tab_), &tab_helper);
  EXPECT_EQ(&tab_helper.GetWebContents(), new_contents);
  EXPECT_NE(&tab_helper.web_contents_content(), old_content);
  EXPECT_EQ(tab_helper.web_contents_content().web_contents(), new_contents);
  EXPECT_THAT(delegates_created_for_,
              testing::ElementsAre(web_contents(), new_contents));
  EXPECT_TRUE(tab_helper.web_contents_content()
                  .GetPrintPreviewExtractionDelegateForTesting());
}

// Observers of the old content learn that it is destroyed, by which time the
// tab helper already serves the new content.
TEST_F(AIChatTabHelperUnitTest, DestroysOldContentAfterReplacingIt) {
  AIChatTabHelper tab_helper(tab_, RecordingDelegateFactory());
  DestroyedObserver observer(tab_helper);
  tab_helper.web_contents_content().AddObserver(&observer);

  content::WebContents* const new_contents = DiscardContents();

  EXPECT_TRUE(observer.destroyed());
  EXPECT_FALSE(observer.destroyed_was_served());
  ASSERT_TRUE(observer.served_when_destroyed());
  EXPECT_EQ(observer.served_when_destroyed(),
            &tab_helper.web_contents_content());
  EXPECT_EQ(observer.served_when_destroyed()->web_contents(), new_contents);
}

TEST_F(AIChatTabHelperUnitTest, NoDelegateAfterDiscardWithoutFactory) {
  AIChatTabHelper tab_helper(
      tab_, AIChatTabHelper::PrintPreviewExtractionDelegateFactory());

  content::WebContents* const new_contents = DiscardContents();

  EXPECT_EQ(tab_helper.web_contents_content().web_contents(), new_contents);
  EXPECT_FALSE(tab_helper.web_contents_content()
                   .GetPrintPreviewExtractionDelegateForTesting());
}

TEST_F(AIChatTabHelperUnitTest, FollowsRepeatedDiscards) {
  AIChatTabHelper tab_helper(tab_, RecordingDelegateFactory());

  content::WebContents* const first_replacement = DiscardContents();
  content::WebContents* const second_replacement = DiscardContents();

  EXPECT_EQ(&tab_helper.GetWebContents(), second_replacement);
  EXPECT_EQ(tab_helper.web_contents_content().web_contents(),
            second_replacement);
  EXPECT_THAT(delegates_created_for_,
              testing::ElementsAre(web_contents(), first_replacement,
                                   second_replacement));
}

// A discard after the tab helper is gone reaches nothing.
TEST_F(AIChatTabHelperUnitTest, StopsFollowingWhenDestroyed) {
  auto tab_helper =
      std::make_unique<AIChatTabHelper>(tab_, RecordingDelegateFactory());
  tab_helper.reset();
  EXPECT_FALSE(AIChatTabHelper::From(&tab_));

  DiscardContents();

  EXPECT_THAT(delegates_created_for_, testing::ElementsAre(web_contents()));
}

}  // namespace ai_chat
