/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_UNITTEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_UNITTEST_HELPER_H_

#include <string>

#include "brave/components/brave_ads/core/internal/history/history_manager_observer.h"
#include "brave/components/brave_ads/core/public/history/history_item_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct AdContentInfo;

class HistoryManagerObserverForTesting final : public HistoryManagerObserver {
 public:
  HistoryManagerObserverForTesting();

  HistoryManagerObserverForTesting(const HistoryManagerObserverForTesting&) =
      delete;
  HistoryManagerObserverForTesting& operator=(
      const HistoryManagerObserverForTesting&) = delete;

  HistoryManagerObserverForTesting(
      HistoryManagerObserverForTesting&&) noexcept = delete;
  HistoryManagerObserverForTesting& operator=(
      HistoryManagerObserverForTesting&&) noexcept = delete;

  ~HistoryManagerObserverForTesting() override;

  bool did_add_history() const { return did_add_history_; }

  const absl::optional<HistoryItemInfo>& history_item() {
    return history_item_;
  }

  bool did_like_ad() const { return did_like_ad_; }

  bool did_dislike_ad() const { return did_dislike_ad_; }

  bool did_like_category() const { return did_like_category_; }

  bool did_dislike_category() const { return did_dislike_category_; }

  bool did_save_ad() const { return did_save_ad_; }

  bool did_unsave_ad() const { return did_unsave_ad_; }

  bool did_mark_ad_as_inappropriate() const {
    return did_mark_ad_as_inappropriate_;
  }

  bool did_mark_ad_as_appropriate() const {
    return did_mark_ad_as_appropriate_;
  }

  void Reset();

 private:
  // HistoryManagerObserver:
  void OnDidAddHistory(const HistoryItemInfo& history_item) override;
  void OnDidLikeAd(const AdContentInfo& ad_content) override;
  void OnDidDislikeAd(const AdContentInfo& ad_content) override;
  void OnDidLikeCategory(const std::string& category) override;
  void OnDidDislikeCategory(const std::string& category) override;
  void OnDidSaveAd(const AdContentInfo& ad_content) override;
  void OnDidUnsaveAd(const AdContentInfo& ad_content) override;
  void OnDidMarkAdAsAppropriate(const AdContentInfo& ad_content) override;
  void OnDidMarkAdAsInappropriate(const AdContentInfo& ad_content) override;

  bool did_add_history_ = false;
  absl::optional<HistoryItemInfo> history_item_;

  bool did_like_ad_ = false;
  bool did_dislike_ad_ = false;

  bool did_like_category_ = false;
  bool did_dislike_category_ = false;

  bool did_save_ad_ = false;
  bool did_unsave_ad_ = false;

  bool did_mark_ad_as_inappropriate_ = false;
  bool did_mark_ad_as_appropriate_ = false;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_UNITTEST_HELPER_H_
