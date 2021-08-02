/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_H_

namespace ads {

template <class T>
class AdEvent {
 public:
  virtual ~AdEvent() = default;

  virtual void FireEvent(const T&) = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_AD_EVENT_H_
