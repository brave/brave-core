// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveVPN
import Combine
import GuardianConnect

/// The current status of the Brave VPN connection
enum VPNStatus: Equatable {
  /// VPN is not connected
  case disconnected
  /// VPN is connected to a given region
  case connected(activeRegion: VPNRegion)
}

/// The VPN region details to show
struct VPNRegion: Equatable {
  /// The unicode/emoji flag for the given country code provided
  var flag: String
  /// The display name for the connected server
  var displayName: String

  init(countryCode: String, displayName: String) {
    self.flag = Self.flagEmojiForCountryCode(code: countryCode)
    self.displayName = displayName
  }

  private static func flagEmojiForCountryCode(code: String) -> String {
    // Regional indicator symbol root Unicode flags index
    let rootIndex: UInt32 = 127397
    var unicodeScalarView = ""

    for scalar in code.unicodeScalars {
      // Shift the letter index to the flags index
      if let appendedScalar = UnicodeScalar(rootIndex + scalar.value) {
        // Append symbol to the Unicode string
        unicodeScalarView.unicodeScalars.append(appendedScalar)
      }
    }
    return unicodeScalarView
  }
}

// MARK: - Live Values

extension VPNRegion {
  init(region: GRDRegion) {
    self.init(countryCode: region.countryISOCode, displayName: region.displayName)
  }
}

extension VPNStatus {
  static var liveVPNStatus: VPNStatus {
    if BraveVPN.isConnected, let region = BraveVPN.activatedRegion.map(VPNRegion.init) {
      return .connected(activeRegion: region)
    }
    return .disconnected
  }
}

extension AnyPublisher {
  static var liveVPNStatus: AnyPublisher<VPNStatus, Never> {
    NotificationCenter.default
      .publisher(for: .NEVPNStatusDidChange)
      .map { _ in
        return .liveVPNStatus
      }
      .eraseToAnyPublisher()
  }
}
