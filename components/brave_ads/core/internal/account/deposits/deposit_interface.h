/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_INTERFACE_H_

#include <string>

#include "base/functional/callback.h"

namespace brave_ads {

using GetDepositCallback = base::OnceCallback<void(bool success, double value)>;

class DepositInterface {
 public:
  virtual ~DepositInterface() = default;

  virtual void GetValue(const std::string& creative_instance_id,
                        GetDepositCallback callback) = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_DEPOSITS_DEPOSIT_INTERFACE_H_
