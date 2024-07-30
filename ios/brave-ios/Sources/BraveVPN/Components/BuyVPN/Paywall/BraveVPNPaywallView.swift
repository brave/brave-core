// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStore
import BraveStrings
import BraveUI
import DesignSystem
import SafariServices
import StoreKit
import SwiftUI
import Then
import os.log

enum BraveVPNPaymentStatus {
  case ongoing
  case success
  case failure
}

struct BraveVPNPaywallView: View {
  @Environment(\.dismiss)
  private var dismiss

  @State
  private var selectedTierType: BraveVPNSubscriptionTier = .monthly

  @State
  private var availableTierTypes: [BraveVPNSubscriptionTier] = [.yearly, .monthly]

  @ObservedObject
  private(set) var storeSDK = BraveStoreSDK.shared

  @State
  private var paymentStatus: BraveVPNPaymentStatus = .success

  @State
  private var isShowingPurchaseAlert = false

  @State
  private var shouldDismiss: Bool = false

  @State
  private var shouldRefreshCredentials = false

  @State
  private var shouldRedeemPromoCode = false

  // Timer used for resetting the restore action to prevent infinite loading
  @State
  private var iapRestoreTimer: Task<Void, Error>?

  var premiumUpgrageSuccessful: ((BraveVPNSubscriptionTier) -> Void)?

  var refreshCredentials: (() -> Void)?

  var body: some View {
    NavigationView {
      VStack(spacing: 8.0) {
        ScrollView {
          VStack(spacing: 16.0) {
            BraveVPNPremiumUpsellView()
              .padding([.top, .horizontal], 8.0)
              .padding(.bottom, 8.0)
            tierSelection
              .padding(.horizontal, 8.0)
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

            ToolbarItemGroup(placement: .cancellationAction) {
              Button("Cancel") {
                dismiss()
              }
              .foregroundColor(.white)
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
          title: Text("Error"),
          message: Text(
            "Unable to complete purchase. Please try again, or check your payment details on Apple and try again."
          ),
          dismissButton: .default(Text("OK"))
        )
      }
      .onChange(of: shouldDismiss) { shouldDismiss in
        premiumUpgrageSuccessful?(selectedTierType)

        if shouldDismiss {
          dismiss()
        }
      }
      .onChange(of: shouldRefreshCredentials) { shouldRefreshCredentials in
        dismiss()
        refreshCredentials?()
      }
    }
    .navigationViewStyle(.stack)
    .onDisappear {
      iapRestoreTimer?.cancel()
    }
  }

  private var tierSelection: some View {
    VStack {
      if availableTierTypes.contains(.yearly) {
        BraveVPNPremiumTierSelectionView(
          title: "One Year",
          description: "SAVE UP TO 25%",
          product: storeSDK.vpnYearlyProduct,
          type: .yearly,
          selectedTierType: $selectedTierType
        )
      }

      if availableTierTypes.contains(.monthly) {
        BraveVPNPremiumTierSelectionView(
          title: "Monthly",
          description: nil,
          product: storeSDK.vpnMonthlyProduct,
          type: .monthly,
          selectedTierType: $selectedTierType
        )
      }

      Text("All subscriptions are auto-renewed but can be cancelled at any time before renewal.")
        .multilineTextAlignment(.center)
        .font(.footnote)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(Color(braveSystemName: .primitivePrimary90))
        .padding([.horizontal], 16.0)
        .padding([.vertical], 12.0)
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
              Text("Try 7 Days Free Subscription")
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
                  Color(UIColor(rgb: 0xFF5500)),
                  Color(UIColor(rgb: 0xFF006B)),
                ]),
              startPoint: .init(x: 0.0, y: 0.0),
              endPoint: .init(x: 0.0, y: 1.0)
            )
          )
        }
      )
      .clipShape(RoundedRectangle(cornerRadius: 16.0, style: .continuous))
      .disabled(paymentStatus == .ongoing)
      .buttonStyle(.plain)
      .padding([.horizontal], 16.0)
    }
  }

  @MainActor
  private func purchaseSubscription() async {
    paymentStatus = .ongoing

    do {
      switch selectedTierType {
      case .monthly:
        try await storeSDK.purchase(product: BraveStoreProduct.vpnMonthly)
      case .yearly:
        try await storeSDK.purchase(product: BraveStoreProduct.vpnYearly)
      }

      paymentStatus = .success
      shouldDismiss = true
    } catch {
      Logger.module.debug("[BraveVPN PaywallView] - Purchase Failed: \(error)")

      paymentStatus = .failure
      isShowingPurchaseAlert = true
    }
  }

  @MainActor
  private func restorePurchase() async {
    paymentStatus = .ongoing

    if await storeSDK.restorePurchases() {
      iapRestoreTimer?.cancel()
      paymentStatus = .success
      shouldDismiss = true
    } else {
      iapRestoreTimer?.cancel()
      paymentStatus = .failure
      isShowingPurchaseAlert = true
    }

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
}

#if DEBUG
#Preview("VPNSubscriptionPaywall") {
  BraveVPNPaywallView()
}
#endif
