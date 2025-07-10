// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Preferences
import Web

extension TabDataValues {
  private struct BraveShieldsTabHelperKey: TabDataKey {
    static var defaultValue: BraveShieldsTabHelper?
  }
  public var braveShieldsHelper: BraveShieldsTabHelper? {
    get { self[BraveShieldsTabHelperKey.self] }
    set { self[BraveShieldsTabHelperKey.self] = newValue }
  }
}

public class BraveShieldsTabHelper {
  private weak var tab: (any TabState)?

  public init(tab: some TabState) {
    self.tab = tab
  }

  @MainActor public func isBraveShieldsEnabled(for url: URL?, isPrivate: Bool) -> Bool {
    guard let url = url ?? tab?.visibleURL else { return false }
    let domain = Domain.getOrCreate(forUrl: url, persistent: isPrivate)
    return !domain.areAllShieldsOff
  }

  @MainActor public func setBraveShieldsEnabled(_ isEnabled: Bool, for url: URL?, isPrivate: Bool) {
    guard let url = url ?? tab?.visibleURL else { return }
    let domain = Domain.getOrCreate(forUrl: url, persistent: isPrivate)
    domain.shield_allOff = NSNumber(booleanLiteral: !isEnabled)
  }

  @MainActor public func shieldLevel(
    for url: URL?,
    isPrivate: Bool,
    considerAllShieldsOption: Bool
  ) -> ShieldLevel {
    guard let url = url ?? tab?.visibleURL else { return .disabled }
    if considerAllShieldsOption {
      return Domain.getOrCreate(forUrl: url, persistent: isPrivate)
        .globalBlockAdsAndTrackingLevel
    } else {
      return Domain.getOrCreate(forUrl: url, persistent: isPrivate)
        .domainBlockAdsAndTrackingLevel
    }
  }

  @MainActor public func setShieldLevel(_ shieldLevel: ShieldLevel, for url: URL?, isPrivate: Bool)
  {
    guard let url = url ?? tab?.visibleURL else { return }
    return Domain.getOrCreate(forUrl: url, persistent: isPrivate)
      .domainBlockAdsAndTrackingLevel = shieldLevel
  }

  @MainActor public func setBlockScriptsEnabled(_ isEnabled: Bool, for url: URL?, isPrivate: Bool) {
    guard let url = url ?? tab?.visibleURL else { return }
    Domain.getOrCreate(forUrl: url, persistent: isPrivate).shield_noScript = NSNumber(
      booleanLiteral: isEnabled
    )
  }

  @MainActor public func setBlockFingerprintingEnabled(
    _ isEnabled: Bool,
    for url: URL?,
    isPrivate: Bool
  ) {
    guard let url = url ?? tab?.visibleURL else { return }
    Domain.getOrCreate(forUrl: url, persistent: isPrivate).shield_fpProtection =
      NSNumber(booleanLiteral: isEnabled)
  }

  /// Whether or not a given shield should be enabled based on domain exceptions and the users global preference
  @MainActor public func isShieldExpected(
    for url: URL?,
    isPrivate: Bool,
    shield: BraveShield,
    considerAllShieldsOption: Bool
  ) -> Bool {
    guard let url = url ?? tab?.visibleURL else { return false }
    let domain = Domain.getOrCreate(forUrl: url, persistent: isPrivate)
    let isShieldOn = { () -> Bool in
      switch shield {
      case .allOff:
        return domain.areAllShieldsOff
      case .fpProtection:
        return domain.shield_fpProtection?.boolValue
          ?? Preferences.Shields.fingerprintingProtection.value
      case .noScript:
        return domain.shield_noScript?.boolValue ?? Preferences.Shields.blockScripts.value
      }
    }()

    let isAllShieldsOff = domain.areAllShieldsOff
    let isSpecificShieldOn = isShieldOn
    return considerAllShieldsOption ? !isAllShieldsOff && isSpecificShieldOn : isSpecificShieldOn
  }

  @MainActor public func shredLevel(
    for url: URL?,
    isPrivate: Bool
  ) -> SiteShredLevel {
    guard let url = url ?? tab?.visibleURL else { return .never }
    return Domain.getOrCreate(forUrl: url, persistent: isPrivate).shredLevel
  }

  @MainActor public func setShredLevel(
    _ shredLevel: SiteShredLevel,
    for url: URL?,
    isPrivate: Bool
  ) {
    guard let url = url ?? tab?.visibleURL else { return }
    Domain.getOrCreate(forUrl: url, persistent: isPrivate).shredLevel = shredLevel
  }
}
