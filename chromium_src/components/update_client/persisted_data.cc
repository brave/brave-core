/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/update_client/persisted_data.cc"

namespace update_client {

bool PersistedDataImpl::BraveGetBool(const std::string& id,
                                     const std::string& key){
    return !GetString(id, key).empty()}

void PersistedDataImpl::BraveSetBool(const std::string& id,
                                     const std::string& key) {
  SetString(id, key, "true");
}

}  // namespace update_client
