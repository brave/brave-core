// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// This structure stores all important data needed to connect to the VPN service using the cross platform credential.
public struct BraveVPNSkusCredential {
  /// The actual credential to pass to the GuardianConnect framework.
  public let guardianCredential: String
  /// Which environment was the `guardianCredential` registered to.
  public let environment: String
  /// How long is the SKU credential valid for.
  /// Important: This date is different from the Guardian's credential expiration date.
  public let expirationDate: Date

  public init(guardianCredential: String, environment: String, expirationDate: Date) {
    self.guardianCredential = guardianCredential
    self.environment = environment
    self.expirationDate = expirationDate
  }
}
