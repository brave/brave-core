/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_P2A_P2A_H_
#define BAT_ADS_INTERNAL_P2A_P2A_H_

#include <string>
#include <vector>

namespace ads {

class AdsImpl;

class P2A {
 public:
  P2A(
      AdsImpl* ads);

  ~P2A();

  void RecordEvent(
      const std::string& name,
      const std::vector<std::string>& value);

 private:
  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_P2A_P2A_H_
