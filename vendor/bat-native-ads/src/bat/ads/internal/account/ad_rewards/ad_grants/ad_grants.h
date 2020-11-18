/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNA_AD_REWARDS_AD_GRANTS_AD_GRANTS_H_
#define BAT_ADS_INTERNA_AD_REWARDS_AD_GRANTS_AD_GRANTS_H_

#include <string>

#include "base/values.h"

namespace ads {

class AdGrants {
 public:
  AdGrants();

  ~AdGrants();

  bool SetFromJson(
      const std::string& json);
  bool SetFromDictionary(
      base::Value* dictionary);

  double GetBalance() const;

 private:
  double balance_ = 0.0;

  bool GetAmountFromDictionary(
      base::DictionaryValue* dictionary,
      double* amount) const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNA_AD_REWARDS_AD_GRANTS_AD_GRANTS_H_
