// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Shared
import UIKit

extension BraveAds {
  public func initialize(_ completion: @escaping (Bool) -> Void) {
    self.initialize(
      with: .init(deviceId: UIDevice.current.identifierForVendor?.uuidString ?? ""),
      buildChannelInfo: .init(
        isRelease: AppConstants.buildChannel == .release,
        name: AppConstants.buildChannel.rawValue
      ),
      completion: completion
    )
  }
  
  @discardableResult
  @MainActor public func initialize() async -> Bool {
    await withCheckedContinuation { c in
      self.initialize { success in
        c.resume(returning: success)
      }
    }
  }
}
