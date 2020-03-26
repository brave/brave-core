/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_PARAMS_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_PARAMS_H_

#include <string>
#include <vector>

namespace brave_rewards {

struct CheckoutDialogParams {
  struct Item {
    std::string sku;
    int quantity;
  };

  CheckoutDialogParams();

  CheckoutDialogParams(
      std::string description,
      double total,
      std::vector<Item> items);

  CheckoutDialogParams(const CheckoutDialogParams&);
  CheckoutDialogParams& operator=(const CheckoutDialogParams&);

  ~CheckoutDialogParams();

  std::string description;
  double total = 0;
  std::vector<Item> items;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_PARAMS_H_
