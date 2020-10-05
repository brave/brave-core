/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// We don't have feed in Brave and thus don't need to create ImageFetcher
#define BRAVE_GET_IMAGE_FETCHER                                              \
  const bool feed_enabled =                                                  \
      base::FeatureList::IsEnabled(feed::kInterestFeedContentSuggestions) || \
      base::FeatureList::IsEnabled(feed::kInterestFeedV2);                   \
  DCHECK(!feed_enabled);                                                     \
  return nullptr;

#include "../../../../../../chrome/browser/offline_pages/prefetch/prefetch_service_factory.cc"
#undef BRAVE_GET_IMAGE_FETCHER
