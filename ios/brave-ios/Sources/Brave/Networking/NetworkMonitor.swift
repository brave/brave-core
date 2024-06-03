// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import Network

public class NetworkMonitor: NSObject {
  private let monitor = NWPathMonitor()
  private let queue = DispatchQueue(label: "com.brave.network-monitor")

  public static let shared = NetworkMonitor()

  private override init() {
    super.init()
  }

  public func start() {
    // Already monitoring
    if monitor.pathUpdateHandler != nil {
      return
    }

    monitor.pathUpdateHandler = { path in
      switch path.status {
      case .satisfied:
        let interfaces = String(
          path.availableInterfaces.map({
            switch $0.type {
            case .other:
              return "Other"
            case .wifi:
              return "Wifi"
            case .cellular:
              return "Cellular"
            case .wiredEthernet:
              return "WiredEthernet"
            case .loopback:
              return "Loopback"
            @unknown default:
              return "Unknown: \($0.name)"
            }
          }).joined(by: ", ")
        )

        DebugLogger.log(
          for: .secureState,
          text: "Network State Changed: Satisfied - Available: \(interfaces)\n"
        )
      case .unsatisfied:
        let reason = path.unsatisfiedReason
        switch reason {
        case .notAvailable:
          DebugLogger.log(
            for: .secureState,
            text: "Network State Changed: Unsatisfied - Reason: notAvailable\n"
          )
        case .cellularDenied:
          DebugLogger.log(
            for: .secureState,
            text: "Network State Changed: Unsatisfied - Reason: cellularDenied\n"
          )
        case .wifiDenied:
          DebugLogger.log(
            for: .secureState,
            text: "Network State Changed: Unsatisfied - Reason: wifiDenied\n"
          )
        case .localNetworkDenied:
          DebugLogger.log(
            for: .secureState,
            text: "Network State Changed: Unsatisfied - Reason: localNetworkDenied\n"
          )
        case .vpnInactive:
          DebugLogger.log(
            for: .secureState,
            text: "Network State Changed: Unsatisfied - Reason: vpnInactive\n"
          )
        @unknown default:
          DebugLogger.log(
            for: .secureState,
            text: "Network State Changed: Unsatisfied - Reason: Unknown => \(reason)\n"
          )
        }
      case .requiresConnection:
        DebugLogger.log(for: .secureState, text: "Network State Changed: RequiresConnection\n")
      @unknown default:
        DebugLogger.log(
          for: .secureState,
          text: "Network State Changed: Unknown => \(path.status)\n"
        )
      }
    }

    monitor.start(queue: queue)
  }
}
