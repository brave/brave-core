/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sessions/content/content_serialized_navigation_driver.h"

#include "components/sessions/core/serialized_navigation_entry.h"
#include "components/sessions/core/serialized_navigation_entry_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/page_state/page_state.h"
#include "third_party/blink/public/common/page_state/page_state_serialization.h"

namespace sessions {

// Tests that PageState data is properly sanitized when post data is present.
TEST(BraveContentSerializedNavigationDriverTest,
     PickleSanitizationWithPostDataForChromePages) {
  ContentSerializedNavigationDriver* driver =
      ContentSerializedNavigationDriver::GetInstance();
  SerializedNavigationEntry navigation =
      SerializedNavigationEntryTestHelper::CreateNavigationForTest();
  ASSERT_TRUE(navigation.has_post_data());

  // When post data is present, the page state should be sanitized.
  EXPECT_EQ(std::string(), driver->GetSanitizedPageStateForPickle(&navigation));

  // Check encoded data is not empty but clean state only with url info for
  // chrome overridable url by extension.
  navigation.set_virtual_url(GURL("chrome://newtab"));
  EXPECT_EQ(blink::PageState::CreateFromURL(navigation.original_request_url())
                .ToEncodedData(),
            driver->GetSanitizedPageStateForPickle(&navigation));

  // Check encoded data is empty.
  navigation.set_virtual_url(GURL("chrome://wallet"));
  EXPECT_EQ(std::string(), driver->GetSanitizedPageStateForPickle(&navigation));
}

// Tests that PageState data is left unsanitized when post data is absent.
TEST(BraveContentSerializedNavigationDriverTest,
     PickleSanitizationNoPostDataForChromePages) {
  ContentSerializedNavigationDriver* driver =
      ContentSerializedNavigationDriver::GetInstance();
  SerializedNavigationEntry navigation =
      SerializedNavigationEntryTestHelper::CreateNavigationForTest();
  SerializedNavigationEntryTestHelper::SetHasPostData(false, &navigation);
  ASSERT_FALSE(navigation.has_post_data());
  EXPECT_EQ(test_data::kEncodedPageState,
            driver->GetSanitizedPageStateForPickle(&navigation));

  // Check encoded data is not empty but clean state only with url info for
  // chrome overridable url by extension.
  navigation.set_virtual_url(GURL("chrome://newtab"));
  EXPECT_EQ(blink::PageState::CreateFromURL(navigation.original_request_url())
                .ToEncodedData(),
            driver->GetSanitizedPageStateForPickle(&navigation));

  // Check encoded data is empty.
  navigation.set_virtual_url(GURL("chrome://wallet"));
  EXPECT_EQ(std::string(), driver->GetSanitizedPageStateForPickle(&navigation));
}

}  // namespace sessions
