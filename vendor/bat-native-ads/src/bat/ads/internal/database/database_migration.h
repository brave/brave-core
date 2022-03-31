/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MIGRATION_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MIGRATION_H_

#include "bat/ads/ads_client_aliases.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace database {

class Migration final {
 public:
  Migration();
  ~Migration();

  void FromVersion(const int from_version, ResultCallback callback);

 private:
  void ToVersion(mojom::DBTransaction* transaction, const int to_version);
};

}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_MIGRATION_H_
