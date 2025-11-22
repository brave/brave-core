// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStore
import Combine
import Foundation
import StoreKit

/// An observable wrapper around AIChatSettingsHelper
@Observable
public class AIChatSettingsViewModel: AIChatSettingsHelperDelegate {
  private let helper: any AIChatSettingsHelper
  private let storeSDK: BraveStoreSDK
  private let skusService: (any SkusSkusService)?
  private var cancellable: AnyCancellable?
  private let calendar: Calendar

  public init(
    helper: any AIChatSettingsHelper,
    skusService: (any SkusSkusService)?,
    calendar: Calendar = .autoupdatingCurrent
  ) {
    self.helper = helper
    self.skusService = skusService
    self.storeSDK = .init(skusService: skusService)
    self.calendar = calendar
    self.modelsWithSubtitles = helper.modelsWithSubtitles

    helper.delegate = self

    Task {
      await updatePremiumStatus()
    }

    cancellable = storeSDK.objectWillChange.sink { [weak self] _ in
      guard let self else { return }
      Task {
        await self.updatePremiumStatus()
      }
    }
  }

  private(set) var modelsWithSubtitles: [AiChat.ModelWithSubtitle]

  var defaultModelWithSubtitle: AiChat.ModelWithSubtitle {
    get {
      modelsWithSubtitles.first(where: { $0.model.key == helper.defaultModelKey })
        ?? modelsWithSubtitles.first!
    }
    set {
      helper.defaultModelKey = newValue.model.key
    }
  }

  func resetData() {
    helper.resetLeoData()
  }

  struct ActivePremiumDetails: Equatable {
    enum Plan {
      case monthly, yearly
    }
    var expirationDate: Date?
    var plan: Plan?
    var isSubscriptionLinkable: Bool = false
  }

  enum PremiumStatus: Equatable {
    case unknown
    case active(ActivePremiumDetails)
    case inactive(_ productsLoaded: Bool)
  }

  var premiumStatus: PremiumStatus = .unknown

  @MainActor func updatePremiumStatus() async {
    let status = await withCheckedContinuation { continuation in
      helper.fetchPremiumStatus { status, _ in
        continuation.resume(returning: status)
      }
    }

    switch status {
    case .active:
      if let transaction = try? storeSDK.leoSubscriptionStatus?.transaction.payloadValue,
        let product = BraveStoreProduct(rawValue: transaction.productID)
      {
        // First attempt to create a premium status based on IAP transaction
        let isYearly = product == .leoYearly
        var expirationDate = transaction.expirationDate
        if expirationDate == nil {
          // Attempt to get the expiration date from the Product itself if the expiration doesn't
          // exist on the transaction for some reason (such as billing retry, or the sub is already
          // expired for a long period of time)
          let product = isYearly ? storeSDK.leoYearlyProduct : storeSDK.leoMonthlyProduct
          if let period = product?.subscription?.subscriptionPeriod,
            let component = Calendar.Component(period.unit)
          {
            expirationDate = calendar.date(byAdding: component, value: period.value, to: .now)
          }
        }
        self.premiumStatus = .active(
          .init(
            expirationDate: expirationDate,
            plan: isYearly ? .yearly : .monthly,
            isSubscriptionLinkable: true
          )
        )
      } else if let credentialSummary = try? await skusService?.credentialsSummary(for: .leo),
        let itemSKU = credentialSummary.order.items.first?.sku,
        let product = BraveStoreProduct(rawValue: itemSKU)
      {
        // If no IAP info is available load a credential summary from the skus service
        let isYearly = product == .leoYearly
        self.premiumStatus = .active(
          .init(
            expirationDate: credentialSummary.expiresAt,
            plan: isYearly ? .yearly : .monthly
          )
        )
      } else {
        // The user has an active sub but we dont have additional info about it (not sure how this
        // is possible)
        self.premiumStatus = .active(.init(expirationDate: nil, plan: nil))
      }
    case .inactive, .activeDisconnected:
      self.premiumStatus = .inactive(storeSDK.isLeoProductsLoaded)
    case .unknown:
      self.premiumStatus = .unknown
    @unknown default:
      self.premiumStatus = .unknown
    }
  }

  var isAppStoreReceiptAvailable: Bool {
    storeSDK.leoSubscriptionStatus?.state != nil
  }

  var isDevReceiptLinkingAvailable: Bool {
    storeSDK.environment != .production
  }

  // MARK: -

  public func defaultModelChanged(fromOldKey oldKey: String, toKey: String) {
    withMutation(keyPath: \.defaultModelWithSubtitle) {}
  }

  public func modelListUpdated() {
    modelsWithSubtitles = self.helper.modelsWithSubtitles
  }
}

extension Calendar.Component {
  fileprivate init?(_ subscriptionPeriodUnit: Product.SubscriptionPeriod.Unit) {
    switch subscriptionPeriodUnit {
    case .day: self = .day
    case .week: self = .weekOfYear
    case .month: self = .month
    case .year: self = .year
    @unknown default:
      return nil
    }
  }
}
