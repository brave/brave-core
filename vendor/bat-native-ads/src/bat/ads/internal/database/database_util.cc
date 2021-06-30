/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/database_util.h"

#include "bat/ads/internal/logging.h"
#include "bat/ads/result.h"

namespace ads {
namespace database {

void OnResultCallback(mojom::DBCommandResponsePtr response,
                      ResultCallback callback) {
  DCHECK(response);

  if (response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    callback(Result::FAILED);
    return;
  }

  callback(Result::SUCCESS);
}

}  // namespace database
}  // namespace ads
