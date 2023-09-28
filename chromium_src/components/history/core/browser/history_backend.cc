/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_sync/features.h"
// Forward include to avoid re-define of URLResult::set_blocked_visit
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/url_row.h"
#include "ui/base/page_transition_types.h"

namespace {

std::u16string GetTransitionString(ui::PageTransition transition_flags) {
  std::u16string transition;
  switch (transition_flags & ui::PAGE_TRANSITION_CORE_MASK) {
#define TRANSITION_CASE(FLAG) \
  case ui::FLAG:              \
    transition = u## #FLAG;   \
    break;

    TRANSITION_CASE(PAGE_TRANSITION_LINK)
    TRANSITION_CASE(PAGE_TRANSITION_TYPED)
    TRANSITION_CASE(PAGE_TRANSITION_AUTO_BOOKMARK)
    TRANSITION_CASE(PAGE_TRANSITION_AUTO_SUBFRAME)
    TRANSITION_CASE(PAGE_TRANSITION_MANUAL_SUBFRAME)
    TRANSITION_CASE(PAGE_TRANSITION_GENERATED)
    TRANSITION_CASE(PAGE_TRANSITION_AUTO_TOPLEVEL)
    TRANSITION_CASE(PAGE_TRANSITION_FORM_SUBMIT)
    TRANSITION_CASE(PAGE_TRANSITION_RELOAD)
    TRANSITION_CASE(PAGE_TRANSITION_KEYWORD)
    TRANSITION_CASE(PAGE_TRANSITION_KEYWORD_GENERATED)

#undef TRANSITION_CASE

    default:
      DCHECK(false);
  }

  return transition;
}

// Taken from TypedURLSyncBridge::ShouldSyncVisit, it's not static, so can't be
// used directly
bool ShouldSyncVisit(int typed_count, ui::PageTransition transition) {
  static const int kTypedUrlVisitThrottleThreshold = 10;
  static const int kTypedUrlVisitThrottleMultiple = 10;

  return (ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_TYPED) &&
          (typed_count < kTypedUrlVisitThrottleThreshold ||
           (typed_count % kTypedUrlVisitThrottleMultiple) == 0));
}

std::u16string GetDiagnosticTitle(const history::URLResult& url_result,
                                  const history::VisitRow& visit) {
  if (base::FeatureList::IsEnabled(
          brave_sync::features::kBraveSyncHistoryDiagnostics)) {
    return base::StrCat(
        {u"ShouldSync:",
         ShouldSyncVisit(url_result.typed_count(), visit.transition) ? u"1"
                                                                     : u"0",
         u" ", std::u16string(u"TypedCount:"),
         base::NumberToString16(url_result.typed_count()), u" ",
         GetTransitionString(visit.transition), u" ", url_result.title()});
  } else {
    return url_result.title();
  }
}

}  // namespace

// It's possible to redefine set_blocked_visit because it appears only once at
// history_backend.cc
#define set_blocked_visit                           \
  set_title(GetDiagnosticTitle(url_result, visit)); \
  url_result.set_blocked_visit


#include "src/components/history/core/browser/history_backend.cc"

#undef set_blocked_visit
