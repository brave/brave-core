/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_DATABASE_DATABASE_VERSION_H_
#define BAT_ADS_INTERNAL_DATABASE_DATABASE_VERSION_H_

#include <stdint.h>

namespace ads {
namespace database {

int32_t version();

int32_t compatible_version();

}  // namespace database
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_DATABASE_DATABASE_VERSION_H_
