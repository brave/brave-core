/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_STATUS_HELPER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_STATUS_HELPER_H_

#include <vector>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace publisher {

// Refreshes the publisher status for each entry in the specified list
void RefreshPublisherStatus(LedgerImpl* ledger,
                            std::vector<mojom::PublisherInfoPtr>&& info_list,
                            ledger::PublisherInfoListCallback callback);

// Refreshes the publisher status for each entry in the specified list
void RefreshPublisherStatus(
    LedgerImpl* ledger,
    std::vector<mojom::PendingContributionInfoPtr>&& list,
    ledger::PendingContributionInfoListCallback callback);

}  // namespace publisher
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PUBLISHER_PUBLISHER_STATUS_HELPER_H_
