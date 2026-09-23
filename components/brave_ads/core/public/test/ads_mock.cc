/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/test/ads_mock.h"

namespace brave_ads {

AdsMock::AdsMock() = default;

AdsMock::~AdsMock() = default;

base::WeakPtr<AdsMock> AdsMock::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace brave_ads
