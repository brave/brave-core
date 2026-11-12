// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import Network

/// Only route and reachability changes invalidate a Tor browsing session.
/// DNS availability, cost and Low Data Mode do not change the SOCKS route.
struct TorNetworkPath: Equatable {
  struct Interface: Hashable {
    let index: Int
    let name: String
  }

  let isSatisfied: Bool
  let interfaces: Set<Interface>
  let preferredInterface: Interface?
  let gateways: Set<NWEndpoint>

  init(isSatisfied: Bool, interfaces: [Interface], gateways: Set<NWEndpoint>) {
    self.isSatisfied = isSatisfied
    self.interfaces = Set(interfaces)
    preferredInterface = interfaces.first
    self.gateways = gateways
  }

  init(_ path: NWPath) {
    self.init(
      isSatisfied: path.status == .satisfied,
      interfaces: path.availableInterfaces
        .filter { path.usesInterfaceType($0.type) }
        .map { Interface(index: $0.index, name: $0.name) },
      gateways: Set(path.gateways)
    )
  }
}

struct TorNetworkPolicy {
  private var previousPath: TorNetworkPath?

  var canConnect: Bool { previousPath?.isSatisfied == true }

  /// Return whether an active session must stop; update the baseline even while
  /// idle so an explicit retry uses the new route.
  mutating func update(_ path: TorNetworkPath) -> Bool {
    let previous = previousPath
    previousPath = path
    guard path.isSatisfied else { return true }
    guard let previous else { return false }
    return path != previous
  }
}
