/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/sharing_hub/brave_sharing_hub_model.h"

namespace sharing_hub {

BraveSharingHubModel::~BraveSharingHubModel() = default;

void BraveSharingHubModel::GetThirdPartyActionList(
    std::vector<SharingHubAction>* list) {
  // We don't use any third party actions now.
}

}  // namespace sharing_hub
