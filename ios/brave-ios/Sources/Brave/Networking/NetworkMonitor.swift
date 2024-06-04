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
      let interfaces = path.availableInterfaces.map({ String(describing: $0.type) }).joined(
        separator: ", "
      )
      DebugLogger.log(
        for: .secureState,
        text:
          """
          Network State Changed: \(String(describing: path.status))
          - Reason: \(path.status == .unsatisfied ? String(describing: path.unsatisfiedReason) : "None")
          - Available: \(String(interfaces))
          """
      )
    }

    monitor.start(queue: queue)
  }
}
