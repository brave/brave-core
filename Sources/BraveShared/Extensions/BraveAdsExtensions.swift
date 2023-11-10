// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Shared
import UIKit

extension BraveAds {
  public func initialize(walletInfo: BraveAds.WalletInfo? = nil, completion: @escaping (Bool) -> Void) {
    self.initService(
      with: .init(deviceId: UIDevice.current.identifierForVendor?.uuidString ?? ""),
      buildChannelInfo: .init(
        isRelease: AppConstants.buildChannel == .release,
        name: AppConstants.buildChannel.rawValue
      ),
      walletInfo: walletInfo,
      completion: completion
    )
  }
  
  @discardableResult
  @MainActor public func initialize(walletInfo: BraveAds.WalletInfo? = nil) async -> Bool {
    await withCheckedContinuation { c in
      self.initialize(walletInfo: walletInfo) { success in
        c.resume(returning: success)
      }
    }
  }
}
