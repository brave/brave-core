// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_PROXIMITY_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_PROXIMITY_PROVIDER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/time/time.h"

namespace brave_transfer {

struct ProximityReading {
  // Identifies the advertiser, such as a discovery tag or a BLE address. Not
  // authenticated.
  std::string peer_hint;
  int rssi = 0;
  base::Time seen_at;
};

// Signal strength of nearby devices ("hold near", not in v1). Readings may
// only be used to rank or highlight devices in the UI, never for trust or
// auto-accept decisions.
class ProximityProvider {
 public:
  using ProximityCallback =
      base::RepeatingCallback<void(const ProximityReading& reading)>;

  virtual ~ProximityProvider() = default;

  virtual bool IsAvailable() const = 0;
  virtual void Start(ProximityCallback on_reading) = 0;
  virtual void Stop() = 0;
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_PROXIMITY_PROVIDER_H_
