// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_TODAY_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_TODAY_SERVICE_H_

#include "components/keyed_service/core/keyed_service.h"
#include "brave/components/p3a/brave_p3a_collector.h"

class PrefService;

namespace brave {
class BraveP3AService;
}  // namespace brave

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

namespace user_prefs {
class PrefRegistrySyncable;
class P
}  // namespace user_prefs

namespace brave_today {

class BraveTodayService : public KeyedService {
 public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  BraveTodayService(brave_ads::AdsService* ads_service,
      PrefService* prefs, PrefService* local_state);
  ~BraveTodayService() override;

  BraveTodayService(const BraveTodayService&) = delete;
  BraveTodayService& operator=(const BraveTodayService&) = delete;

  void RecordUserHasInteracted();
  void RecordItemVisit(int cards_visited_this_session);
  void RecordPromotedItemVisit(
      std::string item_id, std::string creative_instance_id);
  void RecordItemViews(int cards_viewed_this_session);
  void RecordPromotedItemView(
      std::string item_id, std::string creative_instance_id);

 private:
  brave_ads::AdsService* ads_service_;
  PrefService* prefs_ = nullptr;
};

}  // namespace brave_today

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_TODAY_SERVICE_H_
