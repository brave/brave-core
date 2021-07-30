/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/page_transition_util.h"

#include "bat/ads/mojom.h"

namespace ads {

namespace {

PageTransitionType PageTransitionGetCoreValue(const PageTransitionType type) {
  return static_cast<PageTransitionType>(type & ~kPageTransitionQualifierMask);
}

PageTransitionType PageTransitionGetQualifier(const PageTransitionType type) {
  return static_cast<PageTransitionType>(type & kPageTransitionQualifierMask);
}

}  // namespace

bool IsNewNavigation(const PageTransitionType type) {
  return PageTransitionGetQualifier(type) != kPageTransitionForwardBack &&
         PageTransitionGetCoreValue(type) != kPageTransitionReload;
}

bool DidUseBackOrFowardButtonToTriggerNavigation(
    const PageTransitionType type) {
  return PageTransitionGetQualifier(type) == kPageTransitionForwardBack;
}

bool DidUseAddressBarToTriggerNavigation(const PageTransitionType type) {
  return PageTransitionGetQualifier(type) == kPageTransitionFromAddressBar;
}

bool DidNavigateToHomePage(const PageTransitionType type) {
  return PageTransitionGetQualifier(type) == kPageTransitionHomePage;
}

bool DidTransitionFromExternalApplication(const PageTransitionType type) {
  return PageTransitionGetQualifier(type) == kPageTransitionFromAPI;
}

absl::optional<UserActivityEventType> ToUserActivityEventType(
    const PageTransitionType type) {
  const PageTransitionType core_value = PageTransitionGetCoreValue(type);

  switch (core_value) {
    case kPageTransitionLink: {
      return UserActivityEventType::kClickedLink;
    }

    case kPageTransitionTyped: {
      return UserActivityEventType::kTypedUrl;
    }

    case kPageTransitionAutoBookmark: {
      return UserActivityEventType::kClickedBookmark;
    }

    case kPageTransitionGenerated: {
      return UserActivityEventType::kTypedAndSelectedNonUrl;
    }

    case kPageTransitionFormSubmit: {
      return UserActivityEventType::kSubmittedForm;
    }

    case kPageTransitionReload: {
      return UserActivityEventType::kClickedReloadButton;
    }

    case kPageTransitionKeyword: {
      return UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider;
    }

    case kPageTransitionKeywordGenerated: {
      return UserActivityEventType::kGeneratedKeyword;
    }

    default: {
      return absl::nullopt;
    }
  }
}

}  // namespace ads
