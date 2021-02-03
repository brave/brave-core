/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_OPTION_KEYS_H_
#define BRAVELEDGER_OPTION_KEYS_H_

#include <string>

namespace ledger {
namespace option {

const char kPublisherListRefreshInterval[] = "publisher_list_refresh_interval";
const char kClaimUGP[] = "claim_ugp";
const char kContributionsDisabledForBAPMigration[] =
    "contributions_disabled_for_bap_migration";

}  // namespace option
}  // namespace ledger

#endif  // BRAVELEDGER_OPTION_KEYS_H_
