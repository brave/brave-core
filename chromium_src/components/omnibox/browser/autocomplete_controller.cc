/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/omnibox/browser/brave_bookmark_provider.h"
#include "brave/components/omnibox/browser/brave_history_quick_provider.h"
#include "brave/components/omnibox/browser/brave_history_url_provider.h"
#include "brave/components/omnibox/browser/brave_search_provider.h"
#include "brave/components/omnibox/browser/brave_shortcuts_provider.h"
#include "brave/components/omnibox/browser/promotion_provider.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
#include "brave/components/omnibox/browser/suggested_sites_provider.h"
#include "brave/components/omnibox/browser/topsites_provider.h"
#include "components/omnibox/browser/clipboard_provider.h"
#include "components/omnibox/browser/history_cluster_provider.h"
#include "components/omnibox/browser/history_fuzzy_provider.h"

using brave_search_conversion::IsBraveSearchConversionFetureEnabled;

#define SearchProvider BraveSearchProvider
#define HistoryQuickProvider BraveHistoryQuickProvider
#define HistoryURLProvider BraveHistoryURLProvider
#define BookmarkProvider BraveBookmarkProvider
#define ShortcutsProvider BraveShortcutsProvider
#define BRAVE_AUTOCOMPLETE_CONTROLLER_AUTOCOMPLETE_CONTROLLER               \
  providers_.push_back(new TopSitesProvider(provider_client_.get()));       \
  providers_.push_back(new SuggestedSitesProvider(provider_client_.get())); \
  if (IsBraveSearchConversionFetureEnabled() &&                             \
      !provider_client_->IsOffTheRecord())                                  \
    providers_.push_back(new PromotionProvider(provider_client_.get()));

// This sort should be done in the middle of
// AutocompleteController::UpdateResult() because result should be updated
// before notifying at the last of UpdateResult().
#define BRAVE_AUTOCOMPLETE_CONTROLLER_UPDATE_RESULT \
  SortBraveSearchPromotionMatch(&result_);

#include "src/components/omnibox/browser/autocomplete_controller.cc"

#undef BRAVE_AUTOCOMPLETE_CONTROLLER_UPDATE_RESULT
#undef BRAVE_AUTOCOMPLETE_CONTROLLER_AUTOCOMPLETE_CONTROLLER
#undef ShortcutsProvider
#undef BookmarkProvider
#undef HistoryURLProvider
#undef HistoryQuickProvider
#undef SearchProvider
