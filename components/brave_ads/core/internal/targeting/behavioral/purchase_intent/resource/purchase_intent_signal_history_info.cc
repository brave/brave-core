/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"

namespace brave_ads {

PurchaseIntentSignalHistoryInfo::PurchaseIntentSignalHistoryInfo(base::Time at,
                                                                 int weight)
    : at(at), weight(weight) {}

}  // namespace brave_ads
