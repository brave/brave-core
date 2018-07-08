/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/browser/payments/payments_service.h"

#include "base/logging.h"
#include "brave/browser/payments/payments_service_observer.h"

namespace payments {

  PaymentsService::PaymentsService() {
  }

  PaymentsService::~PaymentsService() {
  }

  void PaymentsService::AddObserver(PaymentsServiceObserver* observer) {
    observers_.AddObserver(observer);
  }

  void PaymentsService::RemoveObserver(PaymentsServiceObserver* observer) {
    observers_.RemoveObserver(observer);
  }
}  // namespace payments
