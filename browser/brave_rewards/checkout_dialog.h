/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_H_

#include "brave/browser/brave_rewards/checkout_dialog_controller.h"
#include "brave/browser/brave_rewards/checkout_dialog_params.h"

namespace content {
class WebContents;
}

namespace brave_rewards {

// Displays a tab-modal Brave Rewards checkout dialog. Returns a
// weak reference to a CheckoutDialogController object that allows
// the caller to pass messages (i.e. NotifyPaymentAborted()) to the
// dialog and receive notifications from the dialog. The weak reference
// will be invalidated after the checkout dialog is closed.
base::WeakPtr<CheckoutDialogController> ShowCheckoutDialog(
    content::WebContents* initiator,
    CheckoutDialogParams params);

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_H_
