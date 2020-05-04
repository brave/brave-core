/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_AD_GRANTS_H_
#define BAT_CONFIRMATIONS_INTERNAL_AD_GRANTS_H_

#include <string>

#include "bat/confirmations/confirmations_client.h"

#include "base/values.h"

namespace confirmations {

class ConfirmationsImpl;

class AdGrants {
 public:
  AdGrants();

  ~AdGrants();

  bool SetFromJson(
      const std::string& json);
  bool SetFromDictionary(
      base::DictionaryValue* dictionary);

  double GetBalance() const;

 private:
  double balance_;

  bool GetAmountFromDictionary(
      base::DictionaryValue* dictionary,
      double* amount) const;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_AD_GRANTS_H_
