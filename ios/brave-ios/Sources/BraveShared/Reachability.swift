// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Network

public enum ReachabilityType: CustomStringConvertible {
  case wifi
  case cellular
  case ethernet
  case other
  case offline

  public var description: String {
    switch self {
    case .wifi: return "WiFi"
    case .cellular: return "Cellular"
    case .ethernet: return "Ethernet"
    case .other: return "Other"
    case .offline: return "Offline"
    }
  }
}

open class Reachability: ObservableObject {

  public static let shared = Reachability()

  public struct Status {
    public var isReachable: Bool
    public var connectionType: ReachabilityType
    public var isLowDataMode: Bool
    public var isExpensive: Bool
  }

  @Published
  public private(set) var status: Status

  public var connectionType: ReachabilityType {
    return Self.type(from: monitor.currentPath)
  }

  public var isLowDataMode: Bool {
    return monitor.currentPath.isConstrained
  }

  public var isExpensive: Bool {
    return monitor.currentPath.isExpensive
  }

  public var isOffline: Bool {
    return connectionType == .offline
  }

  // MARK: - Private

  private var monitor: NWPathMonitor
  private let queue = DispatchQueue(label: "com.brave.network-reachability")

  private init() {
    let monitor = NWPathMonitor()

    let type = Self.type(from: monitor.currentPath)
    self.monitor = monitor
    self.status = Status(
      isReachable: type != .offline,
      connectionType: type,
      isLowDataMode: monitor.currentPath.isConstrained,
      isExpensive: monitor.currentPath.isExpensive
    )

    monitor.pathUpdateHandler = { [weak self] path in
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

      let type = Self.type(from: path)

      Task { @MainActor in
        self?.status = Status(
          isReachable: type != .offline,
          connectionType: type,
          isLowDataMode: path.isConstrained,
          isExpensive: path.isExpensive
        )
      }
    }

    monitor.start(queue: queue)
  }

  deinit {
    monitor.cancel()
  }

  private static func type(from path: NWPath) -> ReachabilityType {
    let type: ReachabilityType
    if path.status == .satisfied {
      if path.usesInterfaceType(.wifi) {
        type = .wifi
      } else if path.usesInterfaceType(.cellular) {
        type = .cellular
      } else if path.usesInterfaceType(.wiredEthernet) {
        type = .ethernet
      } else {
        type = .other
      }
    } else {
      type = .offline
    }
    return type
  }
}
