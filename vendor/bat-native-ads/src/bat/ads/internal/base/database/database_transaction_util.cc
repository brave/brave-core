/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/database/database_transaction_util.h"

namespace ads {
namespace database {

void OnResultCallback(mojom::DBCommandResponseInfoPtr response,
                      ResultCallback callback) {
  DCHECK(response);

  if (response->status !=
      mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    callback(/* success */ false);
    return;
  }

  callback(/* success */ true);
}

}  // namespace database
}  // namespace ads
