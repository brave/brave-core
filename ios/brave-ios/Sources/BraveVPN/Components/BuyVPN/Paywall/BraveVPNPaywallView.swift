// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStore
import BraveStrings
import BraveUI
import DesignSystem
import Preferences
import StoreKit
import SwiftUI
import os.log

public struct BraveVPNPaywallView: View {
  @ObservedObject var iapObserverManager: BraveVPNIAPObserverManager

  private let openVPNAuthenticationInNewTab: () -> Void
  private let installVPNProfile: () -> Void

  @State private var selectedTierType: BraveVPNSubscriptionTier = .yearly
  @State private var availableTierTypes: [BraveVPNSubscriptionTier] = [.yearly, .monthly]
  @State private var isShowingPurchaseAlert = false
  @State private var isFreeTrialAvailable = !Preferences.VPN.freeTrialUsed.value
  @State private var iapRestoreTimer: Task<Void, Error>?
  @State private var orientation = UIDeviceOrientation.unknown

  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.sizeCategory) private var sizeCategory

  public init(
    openVPNAuthenticationInNewTab: @escaping (() -> Void),
    installVPNProfile: @escaping () -> Void
  ) {
    self.iapObserverManager = BraveVPNIAPObserverManager(iapObserver: BraveVPN.iapObserver)
    self.openVPNAuthenticationInNewTab = openVPNAuthenticationInNewTab
    self.installVPNProfile = installVPNProfile
  }

  public var body: some View {
    VStack(spacing: 8.0) {
      ScrollView {
        Group {
          if orientation.isLandscape
            || (UIDevice.current.userInterfaceIdiom == .phone && orientation == .portraitUpsideDown)
            || sizeCategory.isAccessibilityCategory
          {
            horizontalContentView
              .padding(24.0)
          } else {
            verticalContentView
              .padding(.horizontal, 16)
          }
        }
      }
      paywallActionView
        .padding(.bottom, 24)
    }
    .navigationTitle(Strings.VPN.vpnName)
    .navigationBarTitleDisplayMode(.inline)
    .background(
      Color(braveSystemName: .primitivePrimary10)
        .edgesIgnoringSafeArea(.all)
    )
    .alert(isPresented: $isShowingPurchaseAlert) {
      Alert(
        title: Text(Strings.VPN.vpnErrorPurchaseFailedTitle),
        message: Text(
          Strings.VPN.vpnErrorPurchaseFailedBody
        ),
        dismissButton: .default(Text(Strings.OKString))
      )
    }
    .onChange(of: iapObserverManager.paymentStatus) { status in
      if case .failure(let error) = status {
        resetTheRestoreTimerIfNecessary()

        if case .transactionError(let err) = error, err?.code == SKError.paymentCancelled {
          return
        }

        isShowingPurchaseAlert = true
      } else if case .success(let receiptValidationRequired) = status {
        resetTheRestoreTimerIfNecessary()

        if receiptValidationRequired {
          Task {
            do {
              _ = try await BraveVPN.validateReceiptData()
            } catch {
              Logger.module.error("Error validating receipt: \(error)")
            }
          }
        }

        presentationMode.dismiss()
        installVPNProfile()
      }
    }
    .onReceive(
      NotificationCenter.default.publisher(for: UIDevice.orientationDidChangeNotification)
    ) { _ in
      let new = UIDevice.current.orientation
      if new != .faceUp && new != .faceDown {
        orientation = new
      }
    }
    .onAppear {
      orientation = UIDevice.current.orientation
    }
    .onDisappear {
      iapRestoreTimer?.cancel()
    }
  }

  private var horizontalContentView: some View {
    HStack {
      VStack(alignment: .leading, spacing: 40) {
        BraveVPNPremiumUpsellView()
        BraveVPNPoweredBrandView(isFreeTrialAvailable: isFreeTrialAvailable)
        Spacer()
      }
      VStack(spacing: 8.0) {
        tierSelection
        BraveVPNSubscriptionActionView(
          refreshCredentials: {
            refreshCredential()
          },
          redeedPromoCode: {
            redeemPromoCode()
          }
        )
      }
    }
  }

  private var verticalContentView: some View {
    VStack(spacing: 8.0) {
      BraveVPNPremiumUpsellView()
        .padding(.top, 24.0)
        .padding(.bottom, 8.0)
      separatorView
        .padding(.horizontal, -16.0)
      BraveVPNPoweredBrandView(isFreeTrialAvailable: isFreeTrialAvailable)
        .padding()
      tierSelection
      BraveVPNSubscriptionActionView(
        refreshCredentials: {
          refreshCredential()
        },
        redeedPromoCode: {
          redeemPromoCode()
        }
      )
      .padding(.bottom, 8.0)
    }
  }

  private var separatorView: some View {
    Color(braveSystemName: .primitivePrimary25)
      .frame(height: 1.0)
  }

  private var tierSelection: some View {
    VStack {
      if availableTierTypes.contains(.yearly) {
        BraveVPNPremiumTierSelectionView(
          originalProduct: BraveVPNProductInfo.yearlySubProduct,
          discountedProduct: BraveVPNProductInfo.monthlySubProduct,
          type: .yearly,
          selectedTierType: $selectedTierType
        )
      }

      if availableTierTypes.contains(.monthly) {
        BraveVPNPremiumTierSelectionView(
          originalProduct: BraveVPNProductInfo.monthlySubProduct,
          discountedProduct: nil,
          type: .monthly,
          selectedTierType: $selectedTierType
        )
      }

      Text(Strings.VPN.paywallDisclaimer)
        .multilineTextAlignment(.leading)
        .font(.footnote)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(Color(braveSystemName: .primitiveBlurple95))
        .padding([.horizontal], 16.0)
        .padding([.top], 12.0)
    }
  }

  private var paywallActionView: some View {
    VStack(spacing: 16) {
      separatorView
      Button {
        addPaymentForSubcription(type: selectedTierType)
      } label: {
        HStack {
          if iapObserverManager.paymentStatus == .ongoing {
            ProgressView()
              .tint(Color.white)
              .padding()
          } else {
            Text(
              isFreeTrialAvailable
                ? Strings.VPN.freeTrialPeriodAction.capitalized
                : Strings.VPN.activateSubscriptionAction.capitalized
            )
            .font(.body.weight(.semibold))
            .foregroundColor(Color(.white))
            .padding()
            .frame(maxWidth: .infinity)
          }
        }
        .background(
          LinearGradient(
            gradient:
              Gradient(colors: [
                Color(UIColor(rgb: 0xFF4000)),
                Color(UIColor(rgb: 0xFF1F01)),
              ]),
            startPoint: .init(x: 0.26, y: 0.0),
            endPoint: .init(x: 0.26, y: 1.0)
          )
        )
      }
      .clipShape(RoundedRectangle(cornerRadius: 12.0, style: .continuous))
      .disabled(iapObserverManager.paymentStatus == .ongoing)
      .padding(.horizontal, 16)
    }
  }

  private func addPaymentForSubcription(type: BraveVPNSubscriptionTier) {
    iapObserverManager.paymentStatus = .ongoing

    var subscriptionProduct: SKProduct?
    switch type {
    case .yearly:
      subscriptionProduct = BraveVPNProductInfo.yearlySubProduct
    case .monthly:
      subscriptionProduct = BraveVPNProductInfo.monthlySubProduct
    }

    guard let subscriptionProduct = subscriptionProduct else {
      Logger.module.error("Failed to retrieve \(type.rawValue) subcription product")
      iapObserverManager.paymentStatus = .failure(nil)
      return
    }

    let payment = SKPayment(product: subscriptionProduct)
    SKPaymentQueue.default().add(payment)
  }

  public func restorePurchase() {
    iapObserverManager.paymentStatus = .ongoing

    SKPaymentQueue.default().restoreCompletedTransactions()

    if iapRestoreTimer != nil {
      iapRestoreTimer?.cancel()
      iapRestoreTimer = nil
    }

    // Adding 30 seconds time-out for restore
    iapRestoreTimer = Task.delayed(bySeconds: 30.0) { @MainActor in
      try Task.checkCancellation()

      iapObserverManager.paymentStatus = .failure(nil)

      // Show Alert for failure of restore
      isShowingPurchaseAlert = true
    }
  }

  private func resetTheRestoreTimerIfNecessary() {
    if iapRestoreTimer != nil {
      iapRestoreTimer?.cancel()
      iapRestoreTimer = nil
    }
  }

  private func refreshCredential() {
    presentationMode.dismiss()
    openVPNAuthenticationInNewTab()
  }

  private func redeemPromoCode() {
    // Open the redeem code sheet
    SKPaymentQueue.default().presentCodeRedemptionSheet()
  }
}

#if DEBUG
#Preview("VPNSubscriptionPaywall") {
  BraveVPNPaywallView(
    openVPNAuthenticationInNewTab: {},
    installVPNProfile: {}
  )
}
#endif
