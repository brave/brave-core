/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/mutated_user_data.h"

#include <utility>

#include "base/values.h"
#include "bat/ads/internal/deprecated/client/client.h"
#include "bat/ads/internal/deprecated/confirmations/confirmations_state.h"

namespace ads {
namespace user_data {

namespace {
constexpr char kMutatedKey[] = "mutated";
}  // namespace

base::DictionaryValue GetMutated() {
  base::DictionaryValue user_data;

  if (ConfirmationsState::Get()->is_mutated() || Client::Get()->is_mutated()) {
    user_data.SetBoolKey(kMutatedKey, true);
  }

  return user_data;
}

}  // namespace user_data
}  // namespace ads
