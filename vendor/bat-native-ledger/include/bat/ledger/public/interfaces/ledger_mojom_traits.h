/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_PUBLIC_INTERFACES_LEDGER_MOJOM_TRAITS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_PUBLIC_INTERFACES_LEDGER_MOJOM_TRAITS_H_

#include "brave/components/sync/protocol/vg_specifics.pb.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger.mojom-shared.h"

namespace mojo {

template <>
struct StructTraits<ledger::mojom::VgSpendStatusDataView,
                    sync_pb::VgSpendStatusSpecifics> {
  static std::uint64_t token_id(
      const sync_pb::VgSpendStatusSpecifics& vg_spend_status) {
    return vg_spend_status.token_id();
  }

  static std::uint64_t redeemed_at(
      const sync_pb::VgSpendStatusSpecifics& vg_spend_status) {
    return vg_spend_status.redeemed_at();
  }

  static ledger::mojom::RewardsType redeem_type(
      const sync_pb::VgSpendStatusSpecifics& vg_spend_status) {
    return static_cast<ledger::mojom::RewardsType>(
        vg_spend_status.redeem_type());
  }

  static bool Read(ledger::mojom::VgSpendStatusDataView data,
                   sync_pb::VgSpendStatusSpecifics* out);
};

}  // namespace mojo

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_PUBLIC_INTERFACES_LEDGER_MOJOM_TRAITS_H_
