/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_EXPERIMENTS_EXPERIMENTS_H_
#define BAT_ADS_INTERNAL_EXPERIMENTS_EXPERIMENTS_H_

#include <stdint.h>

#include <string>
#include <vector>

namespace ads {

class AdsImpl;

class Experiments {
 public:
  explicit Experiments(
      AdsImpl* ads);

  ~Experiments();

  bool IsInitialized();

  bool Initialize(
      const std::string& json);

  void LoadUserModelForLocale(
      const std::string& locale);
  void LoadUserModelForId(
      const std::string& id);

 private:
  bool is_initialized_;
  uint16_t version_ = 0;
  std::vector<Trial> trials_;

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_EXPERIMENTS_EXPERIMENTS_H_
