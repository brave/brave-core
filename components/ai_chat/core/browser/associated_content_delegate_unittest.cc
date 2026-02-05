// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"

#include <string>

#include "base/functional/callback_helpers.h"
#include "base/scoped_observation.h"
#include "brave/components/ai_chat/core/browser/test/mock_associated_content.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace ai_chat {

class AssociatedContentSnapShotObserver
    : public AssociatedContentDelegate::Observer {
 public:
  AssociatedContentSnapShotObserver() = default;
  ~AssociatedContentSnapShotObserver() override = default;

  void Observe(AssociatedContentDelegate* delegate) {
    observation_.Observe(delegate);
  }

  void StopObserving() { observation_.Reset(); }

  // AssociatedContentDelegate::Observer
  MOCK_METHOD(void,
              OnDestroyed,
              (AssociatedContentDelegate * delegate),
              (override));
  MOCK_METHOD(void,
              OnRequestArchive,
              (AssociatedContentDelegate * delegate),
              (override));
  MOCK_METHOD(void,
              OnTitleChanged,
              (AssociatedContentDelegate * delegate),
              (override));

 private:
  base::ScopedObservation<AssociatedContentDelegate,
                          AssociatedContentDelegate::Observer>
      observation_{this};
};

TEST(AssociatedContentDelegateTest, OnNewPage) {
  MockAssociatedContent delegate;
  auto uuid = delegate.uuid();

  // Set initial content, as if we were on a page.
  delegate.SetContentId(5);
  delegate.SetUrl(GURL("https://www.brave.com"));
  delegate.SetTitle(u"Brave");
  delegate.SetTextContent("Content 1");
  delegate.GetContent(base::DoNothing());

  testing::StrictMock<AssociatedContentSnapShotObserver> observer;
  EXPECT_CALL(observer, OnRequestArchive(&delegate))
      .WillOnce([&uuid](AssociatedContentDelegate* delegate) {
        // Observer should get the most up to date content id.
        EXPECT_EQ(delegate->content_id(), 6);

        // The cached content should not have been updated when the observer was
        // called.
        EXPECT_EQ(delegate->title(), u"Brave");
        EXPECT_EQ(delegate->url(), GURL("https://www.brave.com"));
        EXPECT_EQ(delegate->uuid(), uuid);
        EXPECT_EQ(delegate->cached_page_content(),
                  PageContent("Content 1", false));
      });
  observer.Observe(&delegate);

  // Simulate a new page.
  delegate.OnNewPage(6);

  // Delegate should have been cleared.
  EXPECT_EQ(delegate.title(), u"");
  EXPECT_EQ(delegate.url(), GURL::EmptyGURL());
  EXPECT_EQ(delegate.cached_page_content(), PageContent());

  // UUID should not be changed.
  EXPECT_EQ(delegate.uuid(), uuid);
}

TEST(AssociatedContentDelegateTest, DestroyNotificationShouldBeAbleToSnapshot) {
  testing::StrictMock<AssociatedContentSnapShotObserver> observer;
  std::string uuid;
  EXPECT_CALL(observer, OnDestroyed)
      .WillOnce([&uuid, &observer](AssociatedContentDelegate* delegate) {
        // In the destroy notification, the observer should have the most up to
        // date content.
        EXPECT_EQ(delegate->title(), u"Brave");
        EXPECT_EQ(delegate->url(), GURL("https://www.brave.com"));
        EXPECT_EQ(delegate->uuid(), uuid);
        EXPECT_EQ(delegate->cached_page_content(),
                  PageContent("Content 1", false));
        EXPECT_EQ(delegate->content_id(), 5);
        observer.StopObserving();
      });

  {
    MockAssociatedContent delegate;

    // Store a copy of the uuid before it's destroyed, so we can check it was
    // correct in the destroy notification.
    uuid = delegate.uuid();

    // Set initial content, as if we were on a page.
    delegate.SetContentId(5);
    delegate.SetUrl(GURL("https://www.brave.com"));
    delegate.SetTitle(u"Brave");
    delegate.SetTextContent("Content 1");
    delegate.GetContent(base::DoNothing());

    observer.Observe(&delegate);
  }
}

TEST(AssociatedContentDelegateTest, OnTitleChangedShouldProvideNewTitle) {
  MockAssociatedContent delegate;
  testing::StrictMock<AssociatedContentSnapShotObserver> observer;
  EXPECT_CALL(observer, OnTitleChanged)
      .WillOnce([](AssociatedContentDelegate* delegate) {
        EXPECT_EQ(delegate->title(), u"Braverer");
      });

  delegate.SetTitle(u"Brave");
  observer.Observe(&delegate);

  delegate.SetTitle(u"Braverer");

  testing::Mock::VerifyAndClearExpectations(&observer);
}

}  // namespace ai_chat
