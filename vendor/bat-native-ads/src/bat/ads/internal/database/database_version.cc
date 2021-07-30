/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/database_version.h"

namespace ads {
namespace database {

int32_t version() {
  return 15;
}

int32_t compatible_version() {
  return 15;
}

}  // namespace database
}  // namespace ads
