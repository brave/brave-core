/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/legacy_migration_util.h"

#include <string>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

void MaybeDeleteFile(const std::string& name) {
  GetAdsClient().Remove(name,
                        base::BindOnce(
                            [](const std::string& filename, bool success) {
                              if (!success) {
                                BLOG(0, "Failed to delete " << filename);
                              }
                            },
                            name));
}

}  // namespace brave_ads
