// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStore
import Combine
import StoreKit
import SwiftUI

@Observable
public class OriginPaywallViewModel {
  private let store: BraveStoreSDK

  private(set) var isStoreOperationActive: Bool = false
  var isErrorPresented: Bool = false
  private var cancellable: AnyCancellable?

  public init(
    store: BraveStoreSDK,
  ) {
    self.store = store
    // Temporarily set to the yearly product which will be removed/replaced with a one-time purchase
    self.product = store.originYearlyProduct
    cancellable = store.$originYearlyProduct.sink { [weak self] product in
      self?.product = product
    }
  }

  var product: Product?

  func purchase() async {
    isStoreOperationActive = true
    defer { isStoreOperationActive = false }

    do {
      try await store.purchase(product: .originYearly)
    } catch {
      isErrorPresented = true
    }
  }

  func restore() async {
    isStoreOperationActive = true
    defer { isStoreOperationActive = false }

    let success = await withTaskGroup { group in
      group.addTask {
        // 30 seconds time-out for restore
        try? await Task.sleep(for: .seconds(30))
        return false
      }
      group.addTask {
        return await self.store.restorePurchases()
      }
      let result = await group.next() ?? false
      group.cancelAll()
      return result
    }
    if !success {
      isErrorPresented = true
    }
  }
}
