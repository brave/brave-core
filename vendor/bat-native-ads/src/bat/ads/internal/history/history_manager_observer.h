/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"

namespace ads {

struct AdContentInfo;

class HistoryManagerObserver : public base::CheckedObserver {
 public:
  // Invoked when history has changed.
  virtual void OnHistoryDidChange() {}

  // Invoked when liking an ad.
  virtual void OnDidLikeAd(const AdContentInfo& ad_content) {}

  // Invoked when disliking an ad.
  virtual void OnDidDislikeAd(const AdContentInfo& ad_content) {}

  // Invoked when marked to no longer recieve ads for |category|.
  virtual void OnDidMarkToNoLongerReceiveAdsForCategory(
      const std::string& category) {}

  // Invoked when marked to recieve ads for |category|.
  virtual void OnDidMarkToReceiveAdsForCategory(const std::string& category) {}

  // Invoked when an ad is marked as inappropriate.
  virtual void OnDidMarkAdAsInappropriate(const AdContentInfo& ad_content) {}

  // Invoked when an ad is marked as appropriate.
  virtual void OnDidMarkAdAsAppropriate(const AdContentInfo& ad_content) {}

  // Invoked when an ad was saved.
  virtual void OnDidSaveAd(const AdContentInfo& ad_content) {}

  // Invoked when an ad was unsaved.
  virtual void OnDidUnsaveAd(const AdContentInfo& ad_content) {}

 protected:
  ~HistoryManagerObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_H_
