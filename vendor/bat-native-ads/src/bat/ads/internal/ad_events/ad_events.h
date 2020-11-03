/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENTS_H_
#define BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENTS_H_

#include <functional>

#include "bat/ads/result.h"

namespace ads {

class AdsImpl;
class ConfirmationType;
struct AdInfo;
struct AdEventInfo;

using AdEventsCallback = std::function<void(const Result)>;

class AdEvents {
 public:
  AdEvents(
      AdsImpl* ads);

  ~AdEvents();

  void Log(
      const AdInfo& ad,
      const ConfirmationType& confirmation_type,
      AdEventsCallback callback);

  void Log(
      const AdEventInfo& ad_event,
      AdEventsCallback callback);

  void PurgeExpired(
      AdEventsCallback callback);

 private:
  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENTS_H_
