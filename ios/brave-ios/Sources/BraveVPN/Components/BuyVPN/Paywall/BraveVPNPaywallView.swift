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

enum BraveVPNPaymentStatus {
  case ongoing
  case success
  case failure
}

public struct BraveVPNPaywallView: View {
  @Environment(\.dismiss)
  private var dismiss

  @State
  private var selectedTierType: BraveVPNSubscriptionTier = .monthly

  @State
  private var availableTierTypes: [BraveVPNSubscriptionTier] = [.yearly, .monthly]

  @State
  private var paymentStatus: BraveVPNPaymentStatus = .success

  @State
  private var isShowingPurchaseAlert = false

  @State
  private var shouldRefreshCredentials = false

  @State
  private var shouldRedeemPromoCode = false

  @State
  private var isFreeTrialAvailable = !Preferences.VPN.freeTrialUsed.value

  @State
  private var iapRestoreTimer: Task<Void, Error>?

  @ObservedObject
  var iapObserverManager: BraveVPNIAPObserverManager

  var installVPNProfile: (() -> Void)?

  var openVPNAuthenticationInNewTab: (() -> Void)?

  public init(openVPNAuthenticationInNewTab: @escaping (() -> Void)) {
    self.iapObserverManager = BraveVPNIAPObserverManager(iapObserver: BraveVPN.iapObserver)
    self.openVPNAuthenticationInNewTab = openVPNAuthenticationInNewTab
  }

  public var body: some View {
    VStack(spacing: 8.0) {
      ScrollView {
        VStack(spacing: 8.0) {
          BraveVPNPremiumUpsellView()
            .padding([.horizontal, .top], 24.0)
            .padding(.bottom, 8.0)
          Color(braveSystemName: .primitivePrimary25)
            .frame(height: 1.0)
          BraveVPNPoweredBrandView(isFreeTrialAvailable: isFreeTrialAvailable)
          tierSelection
            .padding(.horizontal, 16.0)
          BraveVPNSubscriptionActionView(
            shouldRefreshCredentials: $shouldRefreshCredentials,
            shouldRedeedPromoCode: $shouldRedeemPromoCode
          )
          .padding(.bottom, 8.0)
        }
        .navigationTitle("Brave Firewall + VPN")
        .navigationBarTitleDisplayMode(.inline)
        .toolbar {
          ToolbarItemGroup(placement: .confirmationAction) {
            Button(
              action: {
                Task { await restorePurchase() }
              },
              label: {
                if paymentStatus == .ongoing {
                  ProgressView()
                    .tint(Color.white)
                } else {
                  Text("Restore")
                }
              }
            )
            .foregroundColor(.white)
            .disabled(paymentStatus == .ongoing)
            .buttonStyle(.plain)
          }
        }
      }
      .introspectViewController(customize: { vc in
        vc.navigationItem.do {
          let appearance = UINavigationBarAppearance().then {
            $0.configureWithDefaultBackground()
            $0.backgroundColor = UIColor(braveSystemName: .primitivePrimary10)
            $0.titleTextAttributes = [.foregroundColor: UIColor.white]
            $0.largeTitleTextAttributes = [.foregroundColor: UIColor.white]
          }
          $0.standardAppearance = appearance
          $0.scrollEdgeAppearance = appearance
        }

        vc.navigationController?.navigationBar.tintColor = .white
      })

      paywallActionView
        .padding(.bottom, 16.0)
    }
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
    .onChange(of: shouldRefreshCredentials) { _ in
      dismiss()
      openVPNAuthenticationInNewTab?()
    }
    .onChange(of: shouldRedeemPromoCode) { _ in
      // Open the redeem code sheet
      SKPaymentQueue.default().presentCodeRedemptionSheet()
    }
    .onChange(of: iapObserverManager.isPurchaseSuccessful) { purchaseOrRestoreSuccessful in
      resetTheRestoreTimerIfNecessary()
      
      guard purchaseOrRestoreSuccessful else {
        paymentStatus = .failure
        return
      }
      
      if iapObserverManager.isReceiptValidationRequired {
        Task {
          _ = await BraveVPN.validateReceiptData()
        }
      }
      
      paymentStatus = .success
      
      installVPNProfile?()
    }
    .onChange(of: iapObserverManager.isPurchaseFailedError) { error in
      resetTheRestoreTimerIfNecessary()

      paymentStatus = .failure

      if case .transactionError(let err) = error, err?.code == SKError.paymentCancelled {
        return
      }
      
      isShowingPurchaseAlert = true
    }
    .onDisappear {
      iapRestoreTimer?.cancel()
    }
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

      Text(
        "Subscriptions will be charged via your Apple account. Any unused portion of the free trial, if offered, is forfeited when you buy a subscription.\n\nYour subscription will renew automatically unless it is canceled at least 24 hours before the end of the current period.\n\nYou can manage your subscriptions in Settings.\n\nBy using Brave, you agree to the Terms of Use and Privacy Policy."
      )
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
    VStack(spacing: 16.0) {
      Rectangle()
        .frame(height: 1.0)
        .foregroundColor(Color(braveSystemName: .primitivePrimary25))

      Button(
        action: {
          Task { await purchaseSubscription() }
        },
        label: {
          HStack {
            if paymentStatus == .ongoing {
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
            }
          }
          .frame(maxWidth: .infinity)
          .contentShape(ContainerRelativeShape())
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
      )
      .clipShape(RoundedRectangle(cornerRadius: 12.0, style: .continuous))
      .disabled(paymentStatus == .ongoing)
      .buttonStyle(.plain)
      .padding([.horizontal], 16.0)
    }
  }

  @MainActor
  private func purchaseSubscription() async {
    paymentStatus = .ongoing

    addPaymentForSubcription(type: selectedTierType)
  }

  private func addPaymentForSubcription(type: BraveVPNSubscriptionTier) {
    var subscriptionProduct: SKProduct?

    switch type {
    case .yearly:
      subscriptionProduct = BraveVPNProductInfo.yearlySubProduct
    case .monthly:
      subscriptionProduct = BraveVPNProductInfo.monthlySubProduct
    }

    guard let subscriptionProduct = subscriptionProduct else {
      Logger.module.error("Failed to retrieve \(type.rawValue) subcription product")
      paymentStatus = .failure

      return
    }

    let payment = SKPayment(product: subscriptionProduct)
    SKPaymentQueue.default().add(payment)
  }

  @MainActor
  private func restorePurchase() async {
    paymentStatus = .ongoing

    SKPaymentQueue.default().restoreCompletedTransactions()

    if iapRestoreTimer != nil {
      iapRestoreTimer?.cancel()
      iapRestoreTimer = nil
    }

    // Adding 30 seconds time-out for restore
    iapRestoreTimer = Task.delayed(bySeconds: 30.0) { @MainActor in
      try Task.checkCancellation()

      paymentStatus = .failure

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
}

#if DEBUG
#Preview("VPNSubscriptionPaywall") {
  BraveVPNPaywallView(openVPNAuthenticationInNewTab: {})
}
#endif
