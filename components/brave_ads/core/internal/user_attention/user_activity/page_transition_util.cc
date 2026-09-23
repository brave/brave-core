/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/page_transition_util.h"

#include "ui/base/page_transition_types.h"

namespace brave_ads {

std::optional<UserActivityEventType> ToUserActivityEventType(
    ui::PageTransition page_transition) {
  switch (ui::PageTransitionStripQualifier(page_transition)) {
    case ui::PAGE_TRANSITION_LINK:
      return UserActivityEventType::kClickedLink;

    case ui::PAGE_TRANSITION_TYPED:
      return UserActivityEventType::kTypedUrl;

    case ui::PAGE_TRANSITION_AUTO_BOOKMARK:
      return UserActivityEventType::kClickedBookmark;

    case ui::PAGE_TRANSITION_GENERATED:
      return UserActivityEventType::kTypedAndSelectedNonUrl;

    case ui::PAGE_TRANSITION_FORM_SUBMIT:
      return UserActivityEventType::kSubmittedForm;

    case ui::PAGE_TRANSITION_RELOAD:
      return UserActivityEventType::kClickedReloadButton;

    case ui::PAGE_TRANSITION_KEYWORD:
      return UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider;

    case ui::PAGE_TRANSITION_KEYWORD_GENERATED:
      return UserActivityEventType::kGeneratedKeyword;

    default:
      return std::nullopt;
  }
}

}  // namespace brave_ads
