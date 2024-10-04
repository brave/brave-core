/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_OPERATIONAL_PATTERNS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_OPERATIONAL_PATTERNS_UTIL_H_

#include <string>
#include <string_view>

#include "base/time/time.h"

namespace brave_federated {

int GetCollectionSlot();
std::string CreateCollectionId();

std::u16string FriendlyTime(const base::Time time);

std::string BuildCollectionPingPayload(std::string_view collection_id,
                                       int slot);
std::string BuildDeletePingPayload(std::string_view collection_id);

bool ShouldResetCollectionId(std::string_view collection_id,
                             const base::Time expiration_time);

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_OPERATIONAL_PATTERNS_UTIL_H_
