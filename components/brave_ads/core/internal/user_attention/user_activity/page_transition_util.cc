/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/page_transition_util.h"

namespace brave_ads {

namespace {

PageTransitionType PageTransitionGetCoreValue(PageTransitionType type) {
  return static_cast<PageTransitionType>(type & ~kPageTransitionQualifierMask);
}

PageTransitionType PageTransitionGetQualifier(PageTransitionType type) {
  return static_cast<PageTransitionType>(type & kPageTransitionQualifierMask);
}

}  // namespace

bool IsNewNavigation(PageTransitionType type) {
  return PageTransitionGetQualifier(type) != kPageTransitionForwardBack &&
         PageTransitionGetCoreValue(type) != kPageTransitionReload;
}

bool DidUseBackOrFowardButtonToTriggerNavigation(PageTransitionType type) {
  return PageTransitionGetQualifier(type) == kPageTransitionForwardBack;
}

bool DidUseAddressBarToTriggerNavigation(PageTransitionType type) {
  return PageTransitionGetQualifier(type) == kPageTransitionFromAddressBar;
}

bool DidNavigateToHomePage(PageTransitionType type) {
  return PageTransitionGetQualifier(type) == kPageTransitionHomePage;
}

bool DidTransitionFromExternalApplication(PageTransitionType type) {
  return PageTransitionGetQualifier(type) == kPageTransitionFromAPI;
}

std::optional<UserActivityEventType> ToUserActivityEventType(
    PageTransitionType type) {
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
      return std::nullopt;
    }
  }
}

}  // namespace brave_ads
