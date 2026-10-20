// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_DISCOVERY_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_DISCOVERY_PROVIDER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "brave/components/brave_transfer/core/trust_keys.h"

namespace brave_transfer {

// A host and port to dial. `host` is an IP literal or a resolvable name.
struct PeerEndpoint {
  std::string host;
  uint16_t port = 0;

  bool operator==(const PeerEndpoint&) const = default;
};

// A device that advertised one of the tags a provider was started with. Only
// a hint to dial: the Noise handshake decides whether the device is trusted.
struct PeerCandidate {
  PeerCandidate();
  PeerCandidate(const PeerCandidate&);
  PeerCandidate& operator=(const PeerCandidate&);
  PeerCandidate(PeerCandidate&&);
  PeerCandidate& operator=(PeerCandidate&&);
  ~PeerCandidate();

  // Short provider name, such as "mdns" or "ble".
  std::string provider;
  // Stable for one advertisement, so repeated sightings can be merged.
  std::string instance_id;
  std::vector<PeerEndpoint> endpoints;
  Tag tag{};
  base::Time seen_at;
};

// One way of finding devices of the same Sync chain (mDNS in M1, BLE later).
// The service starts every provider while a brave://transfer page is open.
class DiscoveryProvider {
 public:
  using CandidateCallback =
      base::RepeatingCallback<void(const PeerCandidate& candidate)>;
  using LostCallback =
      base::RepeatingCallback<void(const std::string& instance_id)>;

  virtual ~DiscoveryProvider() = default;

  // Reports candidates advertising a tag `keys` accepts at the current time
  // (ChainKeys::MatchTxtRecord for mDNS). Calling Start() again replaces the
  // keys and callbacks.
  virtual void Start(const ChainKeys& keys,
                     CandidateCallback on_candidate,
                     LostCallback on_lost) = 0;
  virtual void Stop() = 0;
  virtual bool IsStarted() const = 0;
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_CORE_DISCOVERY_PROVIDER_H_
