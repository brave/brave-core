/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_P2A_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_P2A_H_

#include <stdint.h>

#include <string>

class PrefService;
class PrefRegistrySimple;

namespace base {
class DictionaryValue;
}

namespace brave_ads {

class AdsP2A {
 public:
  AdsP2A();
  ~AdsP2A();

  static void RegisterPrefs(
      PrefRegistrySimple* prefs);

  static void RecordEventInWeeklyStorage(
      PrefService* prefs,
      const std::string& pref_name);

  static void EmitAdViewConfirmationHistogram(
      uint64_t number_of_confirmations);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_P2A_H_
