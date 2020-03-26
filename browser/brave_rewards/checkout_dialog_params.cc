/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/checkout_dialog_params.h"

#include <utility>

namespace brave_rewards {

CheckoutDialogParams::CheckoutDialogParams() = default;

CheckoutDialogParams::CheckoutDialogParams(
    std::string description,
    double total,
    std::vector<Item> items)
    : description(description),
      total(total),
      items(std::move(items)) {}

CheckoutDialogParams::CheckoutDialogParams(
    const CheckoutDialogParams&) = default;

CheckoutDialogParams& CheckoutDialogParams::operator=(
    const CheckoutDialogParams&) = default;

CheckoutDialogParams::~CheckoutDialogParams() = default;

}  // namespace brave_rewards
