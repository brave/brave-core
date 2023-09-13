/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/history_manager_observer_unittest_helper.h"

#include "brave/components/brave_ads/core/public/history/ad_content_info.h"

namespace brave_ads {

HistoryManagerObserverForTesting::HistoryManagerObserverForTesting() = default;

HistoryManagerObserverForTesting::~HistoryManagerObserverForTesting() = default;

void HistoryManagerObserverForTesting::OnDidAddHistory(
    const HistoryItemInfo& history_item) {
  did_add_history_ = true;
  history_item_ = history_item;
}

void HistoryManagerObserverForTesting::OnDidLikeAd(
    const AdContentInfo& /*ad_content*/) {
  did_like_ad_ = true;
}

void HistoryManagerObserverForTesting::OnDidDislikeAd(
    const AdContentInfo& /*ad_content*/) {
  did_dislike_ad_ = true;
}

void HistoryManagerObserverForTesting::OnDidLikeCategory(
    const std::string& /*category*/) {
  did_like_category_ = true;
}

void HistoryManagerObserverForTesting::OnDidDislikeCategory(
    const std::string& /*category*/) {
  did_dislike_category_ = true;
}

void HistoryManagerObserverForTesting::OnDidSaveAd(
    const AdContentInfo& /*ad_content*/) {
  did_save_ad_ = true;
}

void HistoryManagerObserverForTesting::OnDidUnsaveAd(
    const AdContentInfo& /*ad_content*/) {
  did_unsave_ad_ = true;
}

void HistoryManagerObserverForTesting::OnDidMarkAdAsAppropriate(
    const AdContentInfo& /*ad_content*/) {
  did_mark_ad_as_appropriate_ = true;
}

void HistoryManagerObserverForTesting::OnDidMarkAdAsInappropriate(
    const AdContentInfo& /*ad_content*/) {
  did_mark_ad_as_inappropriate_ = true;
}

void HistoryManagerObserverForTesting::Reset() {
  did_add_history_ = false;
  history_item_.reset();

  did_like_ad_ = false;
  did_dislike_ad_ = false;

  did_like_category_ = false;
  did_dislike_category_ = false;

  did_save_ad_ = false;
  did_unsave_ad_ = false;

  did_mark_ad_as_inappropriate_ = false;
  did_mark_ad_as_appropriate_ = false;
}

}  // namespace brave_ads
