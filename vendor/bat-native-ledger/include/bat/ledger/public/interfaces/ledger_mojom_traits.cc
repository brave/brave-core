/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<ledger::mojom::VgSpendStatusDataView,
                  sync_pb::VgSpendStatusSpecifics>::
    Read(ledger::mojom::VgSpendStatusDataView data,
         sync_pb::VgSpendStatusSpecifics* out) {
  out->set_token_id(data.token_id());
  out->set_redeemed_at(data.redeemed_at());
  out->set_redeem_type(static_cast<std::int32_t>(data.redeem_type()));

  return true;
}

}  // namespace mojo
