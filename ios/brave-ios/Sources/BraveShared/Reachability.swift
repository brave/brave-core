// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Network
import Preferences
import Shared

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

  fileprivate init(path: NWPath) {
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
    self = type
  }
}

@Observable
public class Reachability {

  public static let shared = Reachability()

  public struct Status {
    public var connectionType: ReachabilityType
    public var isLowDataMode: Bool
    public var isExpensive: Bool
  }

  public private(set) var status: Status

  // MARK: - Private

  private let monitor = NWPathMonitor()
  private let queue = DispatchQueue(label: "com.brave.network-reachability")

  private init() {
    let type = ReachabilityType(path: monitor.currentPath)
    self.status = Status(
      connectionType: type,
      isLowDataMode: monitor.currentPath.isConstrained,
      isExpensive: monitor.currentPath.isExpensive
    )

    monitor.pathUpdateHandler = { [weak self] path in
      if !AppConstants.isOfficialBuild || Preferences.Debug.developerOptionsEnabled.value {
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

      let type = ReachabilityType(path: path)

      Task { @MainActor in
        self?.status = Status(
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
}
