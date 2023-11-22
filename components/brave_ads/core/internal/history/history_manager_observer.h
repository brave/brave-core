/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"

namespace brave_ads {

struct AdContentInfo;
struct HistoryItemInfo;

class HistoryManagerObserver : public base::CheckedObserver {
 public:
  // Invoked when history is added.
  virtual void OnDidAddHistory(const HistoryItemInfo& history_item) {}

  // Invoked when the user likes an ad.
  virtual void OnDidLikeAd(const AdContentInfo& ad_content) {}

  // Invoked when the user dislikes an ad.
  virtual void OnDidDislikeAd(const AdContentInfo& ad_content) {}

  // Invoked when the likes a `category`.
  virtual void OnDidLikeCategory(const std::string& category) {}

  // Invoked when the user dislikes a `category`.
  virtual void OnDidDislikeCategory(const std::string& category) {}

  // Invoked when a user saves an ad.
  virtual void OnDidSaveAd(const AdContentInfo& ad_content) {}

  // Invoked when a user unsaves an ad.
  virtual void OnDidUnsaveAd(const AdContentInfo& ad_content) {}

  // Invoked when a user marks an ad as appropriate.
  virtual void OnDidMarkAdAsAppropriate(const AdContentInfo& ad_content) {}

  // Invoked when a user marks an ad as inappropriate.
  virtual void OnDidMarkAdAsInappropriate(const AdContentInfo& ad_content) {}
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_HISTORY_MANAGER_OBSERVER_H_
