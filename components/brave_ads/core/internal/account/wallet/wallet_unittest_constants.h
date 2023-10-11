/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_WALLET_WALLET_UNITTEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_WALLET_WALLET_UNITTEST_CONSTANTS_H_

namespace brave_ads {

constexpr char kWalletPaymentId[] = "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7";

constexpr char kWalletRecoverySeed[] =
    "x5uBvgI5MTTVY6sjGv65e9EHr8v7i+UxkFB9qVc5fP0=";
constexpr char kInvalidWalletRecoverySeed[] =
    "y6vCwhJ6NUUWZ7tkHw76f0FIs9w8j-VylGC0rWd6gQ1=";

constexpr char kWalletPublicKey[] =
    "BiG/i3tfNLSeOA9ZF5rkPCGyhkc7KCRbQS3bVGMvFQ0=";

constexpr char kWalletSecretKey[] =
    R"(kwUjEEdzI6rkI6hLoyxosa47ZrcZUvbYppAm4zvYF5gGIb+Le180tJ44D1kXmuQ8IbKGRzsoJFtBLdtUYy8VDQ==)";

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_WALLET_WALLET_UNITTEST_CONSTANTS_H_
