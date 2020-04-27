/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/payments/content/bat_payment_app.h"

#define BRAVE_SERVICE_WORKER_PAYMENT_APP_IGNORED_METHODS \
    kBatPaymentMethod

#include "../../../../../components/payments/content/service_worker_payment_app_finder.cc"

#undef BRAVE_SERVICE_WORKER_PAYMENT_APP_IGNORED_METHODS
