// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_TRANSFER_MDNS_DISCOVERY_PROVIDER_H_
#define BRAVE_BROWSER_BRAVE_TRANSFER_MDNS_DISCOVERY_PROVIDER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_transfer/core/discovery_provider.h"
#include "chrome/browser/local_discovery/service_discovery_client.h"
#include "chrome/browser/local_discovery/service_discovery_device_lister.h"

namespace local_discovery {
class ServiceDiscoverySharedClient;
}  // namespace local_discovery

namespace brave_transfer {

// DNS-SD service type browsed for (PROTOCOL.md §2), in the form the
// local_discovery listers expect.
inline constexpr char kBraveTransferServiceType[] = "_brave-xfer._tcp.local";

// Parses "key=value" TXT strings. Entries without "=" and repeated keys after
// the first are ignored, as RFC 6763 §6.4 requires.
base::flat_map<std::string, std::string> ParseTxtRecord(
    const std::vector<std::string>& metadata);

// Returns the candidate for a browsed service if it advertises a tag `keys`
// accepts at `now`.
std::optional<PeerCandidate> CandidateFromService(
    const local_discovery::ServiceDescription& service,
    const ChainKeys& keys,
    base::Time now);

// Browses the local network for Brave Transfer advertisements (iOS devices
// advertise; desktop only browses in M1).
class MdnsDiscoveryProvider
    : public DiscoveryProvider,
      public local_discovery::ServiceDiscoveryDeviceLister::Delegate {
 public:
  // How often seen services are re-checked against the current epoch's tags
  // and the network is queried again.
  static constexpr base::TimeDelta kRefreshInterval = base::Minutes(1);

  MdnsDiscoveryProvider();
  MdnsDiscoveryProvider(const MdnsDiscoveryProvider&) = delete;
  MdnsDiscoveryProvider& operator=(const MdnsDiscoveryProvider&) = delete;
  ~MdnsDiscoveryProvider() override;

  // DiscoveryProvider:
  void Start(const ChainKeys& keys,
             CandidateCallback on_candidate,
             LostCallback on_lost) override;
  void Stop() override;
  bool IsStarted() const override;

 private:
  // local_discovery::ServiceDiscoveryDeviceLister::Delegate:
  void OnDeviceChanged(
      const std::string& service_type,
      bool added,
      const local_discovery::ServiceDescription& service_description) override;
  void OnDeviceRemoved(const std::string& service_type,
                       const std::string& service_name) override;
  void OnDeviceCacheFlushed(const std::string& service_type) override;
  void OnPermissionRejected() override;

  void Evaluate(const local_discovery::ServiceDescription& service);
  void ReportLost(const std::string& service_name);
  void Refresh();

  std::optional<ChainKeys> keys_;
  CandidateCallback on_candidate_;
  LostCallback on_lost_;
  scoped_refptr<local_discovery::ServiceDiscoverySharedClient> client_;
  std::unique_ptr<local_discovery::ServiceDiscoveryDeviceLister> lister_;
  base::flat_map<std::string, local_discovery::ServiceDescription> services_;
  base::flat_set<std::string> reported_;
  base::RepeatingTimer refresh_timer_;
};

}  // namespace brave_transfer

#endif  // BRAVE_BROWSER_BRAVE_TRANSFER_MDNS_DISCOVERY_PROVIDER_H_
