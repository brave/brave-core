// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TRUST_SOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TRUST_SOURCE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/observer_list_types.h"
#include "brave/components/brave_transfer/common/brave_transfer.mojom.h"
#include "brave/components/brave_transfer/core/trust_keys.h"

namespace brave_transfer {

// A device in this device's Brave Sync chain.
struct ChainDevice {
  std::string guid;
  std::string name;
  mojom::DeviceType type = mojom::DeviceType::kUnknown;

  bool operator==(const ChainDevice&) const = default;
};

// Where Sync-chain trust comes from (design plan §3.3). Implemented on top of
// Brave Sync in the browser; faked in tests.
class TrustSource {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // Sync was turned on or off, or the chain's device list changed.
    virtual void OnTrustChanged() = 0;
  };

  virtual ~TrustSource() = default;

  // Whether Sync is active. Chain trust only applies while it is.
  virtual bool IsSyncActive() const = 0;
  // Keys derived from the chain's sync code, or nullopt while Sync is not
  // active or the code cannot be read.
  virtual std::optional<ChainKeys> GetChainKeys() const = 0;
  // This device's sync device GUID and name, empty while Sync is not active.
  virtual std::string GetLocalDeviceGuid() const = 0;
  virtual std::string GetLocalDeviceName() const = 0;
  // The other devices in the chain.
  virtual std::vector<ChainDevice> GetChainDevices() const = 0;

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TRUST_SOURCE_H_
