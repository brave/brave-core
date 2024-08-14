/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_TEST_UTIL_H_

#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

struct TransactionInfo;

namespace test {

TransactionInfo BuildTransaction(double value,
                                 AdType ad_type,
                                 ConfirmationType confirmation_type,
                                 base::Time reconciled_at,
                                 bool should_generate_random_uuids);
TransactionInfo BuildUnreconciledTransaction(double value,
                                             AdType ad_type,
                                             ConfirmationType confirmation_type,
                                             bool should_generate_random_uuids);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_TEST_UTIL_H_
