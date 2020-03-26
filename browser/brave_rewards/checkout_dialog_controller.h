/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_CONTROLLER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_CONTROLLER_H_

#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"

namespace brave_rewards {

// Defines the interface between the opener of the checkout dialog
// and the classes responsible for managing the behavior of the
// checkout dialog.
class CheckoutDialogController
    : public base::SupportsWeakPtr<CheckoutDialogController> {

 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPaymentAborted() = 0;
    virtual void OnPaymentConfirmed() = 0;
  };

  CheckoutDialogController();
  ~CheckoutDialogController();

  CheckoutDialogController(
      const CheckoutDialogController&) = delete;
  CheckoutDialogController& operator=(
      const CheckoutDialogController&) = delete;

  // Notifies the checkout dialog that the payment has been
  // aborted by the initiator.
  void NotifyPaymentAborted();

  // Notifies the checkout dialog that the payment has been
  // confirmed by the initiator.
  void NotifyPaymentConfirmed();

  using OnDialogClosedCallback =
      base::OnceCallback<void(bool payment_confirmed)>;

  // Sets a callback that will be run when the checkout dialog
  // is closed.
  void SetOnDialogClosedCallback(OnDialogClosedCallback callback);

  using OnPaymentReadyCallback =
      base::OnceCallback<void(const std::string& order_id)>;

  // Sets a callback that will be run when the payment has been
  // accepted and is waiting for confirmation by the initiator.
  // The initiator is expected to call NotifyPaymentConfirmed()
  // at some point after running this callback so that the user
  // can be informed that the transaction was successful.
  void SetOnPaymentReadyCallback(OnPaymentReadyCallback callback);

 private:
  friend class CheckoutDialogMessageHandler;
  friend class CheckoutDialogDelegate;

  // Internal methods for use by checkout dialog classes:
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  void NotifyDialogClosed();
  void NotifyPaymentReady(const std::string& order_id);

  bool payment_confirmed_ = false;
  base::ObserverList<Observer> observers_;
  OnDialogClosedCallback dialog_closed_callback_;
  OnPaymentReadyCallback payment_ready_callback_;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_CHECKOUT_DIALOG_CONTROLLER_H_
