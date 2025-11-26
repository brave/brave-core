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

enum AIChatPaymentStatus {
  case ongoing
  case success
  case failure
}

public enum AIChatSubscriptionTier {
  case monthly
  case yearly
}

public struct AIChatPaywallView: View {
  @Environment(\.dismiss)
  private var dismiss

  @Environment(\.allowExternalPurchaseLinks) private var allowExternalPurchaseLinks
  @Environment(\.pixelLength) private var pixelLength

  @State
  private var selectedTierType: AIChatSubscriptionTier = .monthly

  @State
  private var availableTierTypes: [AIChatSubscriptionTier] = [.monthly, .yearly]

  @ObservedObject
  private(set) var storeSDK = BraveStoreSDK.shared

  @State
  private var paymentStatus: AIChatPaymentStatus = .success

  @State
  private var isMonthlyIntroOfferAvailable: Bool = false

  @State
  private var isYearlyIntroOfferAvailable: Bool = false

  @State
  private var isShowingPurchaseAlert = false

  @State
  private var shouldDismiss: Bool = false

  // Timer used for resetting the restore action to prevent infinite loading
  @State
  private var iapRestoreTimer: Task<Void, Error>?

  var premiumUpgrageSuccessful: ((AIChatSubscriptionTier) -> Void)?

  var refreshCredentials: (() -> Void)?
  var openDirectCheckout: (() -> Void)?

  public init(
    premiumUpgrageSuccessful: ((AIChatSubscriptionTier) -> Void)? = nil,
    refreshCredentials: (() -> Void)? = nil,
    openDirectCheckout: (() -> Void)? = nil
  ) {
    self.premiumUpgrageSuccessful = premiumUpgrageSuccessful
    self.refreshCredentials = refreshCredentials
    self.openDirectCheckout = openDirectCheckout
  }

  public var body: some View {
    NavigationView {
      VStack(spacing: 0) {
        ScrollView {
          VStack(spacing: 16.0) {
            PremiumUpsellTitleView(
              upsellType: .premium,
              isPaywallPresented: true
            )
            PremiumUpsellDetailView(isPaywallPresented: true)
              .padding([.top, .horizontal], 8.0)
              .padding(.bottom, 8.0)
            tierSelection
              .padding(.horizontal, 8.0)
            VStack(spacing: 16) {
              AIChatActionButton(
                title: Strings.Paywall.alreadyPurchasedTitle,
                label: Strings.Paywall.refreshCredentialsButtonTitle,
                action: {
                  dismiss()
                  refreshCredentials?()
                }
              )
            }
          }
          .padding(.vertical, 16.0)
          .navigationTitle(Strings.AIChat.paywallViewTitle)
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
                    Text(Strings.AIChat.restorePaywallButtonTitle)
                  }
                }
              )
              .foregroundColor(.white)
              .disabled(paymentStatus == .ongoing)
              .buttonStyle(.plain)
            }

            ToolbarItemGroup(placement: .cancellationAction) {
              Button(Strings.CancelString) {
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

        paywallActionContainerView
      }
      .background(
        Color(braveSystemName: .primitivePrimary10)
          .ignoresSafeArea()
          .overlay(
            Image("leo-product", bundle: .module),
            alignment: .topTrailing
          )
      )
      .alert(isPresented: $isShowingPurchaseAlert) {
        Alert(
          title: Text(Strings.genericErrorTitle),
          message: Text(Strings.AIChat.paywallPurchaseErrorDescription),
          dismissButton: .default(Text(Strings.OKString))
        )
      }
      .onChange(of: shouldDismiss) { shouldDismiss in
        premiumUpgrageSuccessful?(selectedTierType)

        if shouldDismiss {
          dismiss()
        }
      }
    }
    .navigationViewStyle(.stack)
    .onDisappear {
      iapRestoreTimer?.cancel()
    }
    .task {
      await fetchIntroOfferStatus()
    }
  }

  private var tierSelection: some View {
    VStack {
      if availableTierTypes.contains(.yearly) {
        AIChatPremiumTierSelectionView(
          title: Strings.AIChat.paywallYearlySubscriptionTitle,
          description: Strings.AIChat.paywallYearlySubscriptionDescription,
          product: storeSDK.leoYearlyProduct,
          type: .yearly,
          selectedTierType: $selectedTierType
        )
      }

      if availableTierTypes.contains(.monthly) {
        AIChatPremiumTierSelectionView(
          title: Strings.AIChat.paywallMontlySubscriptionTitle,
          description: nil,
          product: storeSDK.leoMonthlyProduct,
          type: .monthly,
          selectedTierType: $selectedTierType
        )
      }

      Text(Strings.AIChat.paywallPurchaseDeepNote)
        .multilineTextAlignment(.center)
        .font(.footnote)
        .frame(maxWidth: .infinity, alignment: .leading)
        .fixedSize(horizontal: false, vertical: true)
        .foregroundStyle(Color(braveSystemName: .primitivePrimary90))
        .padding([.horizontal], 16.0)
        .padding([.vertical], 12.0)
    }
  }

  @ViewBuilder private var paywallActionContainerView: some View {
    if allowExternalPurchaseLinks {
      externalPurchasesAllowedActionView
    } else {
      standardPaywallActionView
    }
  }

  private var externalPurchasesAllowedActionView: some View {
    VStack(spacing: 16) {
      VStack(spacing: 0) {
        Text(Strings.Paywall.startTrialTitle)
          .font(.callout.weight(.semibold))
        Text(Strings.Paywall.startTrialSubtitle)
          .font(.footnote)
      }
      .multilineTextAlignment(.center)
      VStack {
        Button {
          Task { await purchaseSubscription() }
        } label: {
          HStack {
            Text(Strings.Paywall.appStoreCheckoutOptionTitle)
              .font(.callout.weight(.semibold))
              .multilineTextAlignment(.leading)
            Spacer()
            Text(Strings.Paywall.appStoreCheckoutOptionSubtitle)
              .font(.footnote)
              .multilineTextAlignment(.trailing)
          }
          .opacity(paymentStatus == .ongoing ? 0 : 1)
          .overlay {
            if paymentStatus == .ongoing {
              ProgressView()
                .tint(Color.white)
                .padding()
            }
          }
          .frame(maxWidth: .infinity)
          .padding(12)
          .frame(minHeight: 52)
          .background(
            Color(braveSystemName: .primitiveBlurple35),
            in: .rect(cornerRadius: 12, style: .continuous)
          )
        }
        .disabled(paymentStatus == .ongoing)
        Button {
          dismiss()
          openDirectCheckout?()
        } label: {
          HStack {
            Text(Strings.Paywall.braveAccountCheckoutOptionTitle)
              .font(.callout.weight(.semibold))
              .multilineTextAlignment(.leading)
            Spacer()
            Text(
              LocalizedStringKey(
                String.localizedStringWithFormat(
                  Strings.Paywall.braveAccountCheckoutOptionSubtitle,
                  ExternalPurchaseLinksSupport.discountCode,
                  ExternalPurchaseLinksSupport.discountAmount.formatted(.percent)
                )
              )
            )
            .multilineTextAlignment(.trailing)
            .font(.footnote)
          }
          .fontWeight(.semibold)
          .frame(maxWidth: .infinity)
          .padding(12)
          .frame(minHeight: 52)
          .background(
            LinearGradient(
              colors: [
                Color(braveSystemName: .primitiveBrandsRorange2),
                Color(braveSystemName: .primitiveBrandsRorange3),
              ],
              startPoint: .top,
              endPoint: .bottom
            ),
            in: .rect(cornerRadius: 12, style: .continuous)
          )
        }
      }
    }
    .foregroundStyle(Color.white)
    .frame(maxWidth: .infinity)
    .padding(16)
    .background(
      LinearGradient(
        colors: [
          Color(braveSystemName: .primitiveBlurple20),
          Color(braveSystemName: .primitiveBlurple10),
        ],
        startPoint: .top,
        endPoint: .bottom
      )
      .shadow(.drop(color: Color(braveSystemName: .primitiveBlurple5), radius: 16)),
      in: .rect(
        topLeadingRadius: 16,
        bottomLeadingRadius: 0,
        bottomTrailingRadius: 0,
        topTrailingRadius: 16,
        style: .continuous
      )
    )
    .dynamicTypeSize(DynamicTypeSize.xSmall..<DynamicTypeSize.accessibility1)
    .fixedSize(horizontal: false, vertical: true)
  }

  private var standardPaywallActionView: some View {
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
            let isIntroOfferAvailable =
              (selectedTierType == .monthly && isMonthlyIntroOfferAvailable)
              || (selectedTierType == .yearly && isYearlyIntroOfferAvailable)

            Text(
              isIntroOfferAvailable
                ? Strings.AIChat.paywallPurchaseActionIntroOfferTitle
                : Strings.AIChat.paywallPurchaseActionTitle
            )
            .font(.headline)
            .foregroundColor(Color(.white))
            .padding()
          }
        }
        .frame(maxWidth: .infinity)
        .contentShape(ContainerRelativeShape())
        .background(
          LinearGradient(
            colors: [
              Color(braveSystemName: .primitiveBrandsRorange2),
              Color(braveSystemName: .primitiveBrandsRorange3),
            ],
            startPoint: .top,
            endPoint: .bottom
          ),
          in: .rect(cornerRadius: 12, style: .continuous)
        )
      }
    )
    .disabled(paymentStatus == .ongoing)
    .buttonStyle(.plain)
    .padding(16)
    .overlay(alignment: .top) {
      Color(braveSystemName: .primitivePrimary25)
        .frame(height: pixelLength)
    }
  }

  @MainActor
  private func purchaseSubscription() async {
    paymentStatus = .ongoing

    do {
      switch selectedTierType {
      case .monthly:
        try await storeSDK.purchase(product: BraveStoreProduct.leoMonthly)
      case .yearly:
        try await storeSDK.purchase(product: BraveStoreProduct.leoYearly)
      }

      paymentStatus = .success
      shouldDismiss = true
    } catch {
      Logger.module.debug("[AIChatPaywallView] - Purchase Failed: \(error)")

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

  private func fetchIntroOfferStatus() async {
    paymentStatus = .ongoing

    isMonthlyIntroOfferAvailable = await storeSDK.isIntroOfferAvailable(
      for: BraveStoreProduct.leoMonthly
    )

    isYearlyIntroOfferAvailable = await storeSDK.isIntroOfferAvailable(
      for: BraveStoreProduct.leoYearly
    )

    paymentStatus = .success
  }
}

private struct AIChatPremiumTierSelectionView: View {
  var title: String
  var description: String?
  var product: Product?
  var type: AIChatSubscriptionTier

  @Binding
  var selectedTierType: AIChatSubscriptionTier

  var body: some View {
    Button(
      action: {
        selectedTierType = type
      },
      label: {
        HStack {
          VStack(alignment: .leading, spacing: 8.0) {
            Text(title)
              .font(.title2.weight(.semibold))
              .foregroundColor(Color(.white))

            if let description = description {
              Text(description)
                .font(.caption2.weight(.semibold))
                .foregroundColor(Color(braveSystemName: .green50))
                .padding(4.0)
                .background(Color(braveSystemName: .green20))
                .clipShape(RoundedRectangle(cornerRadius: 4.0, style: .continuous))
            }
          }

          Spacer()

          if let product = product {
            HStack(alignment: .center, spacing: 2) {
              Text(
                "\(product.priceFormatStyle.locale.currency?.identifier ?? "")\(product.priceFormatStyle.locale.currencySymbol ?? "")"
              )
              .font(.subheadline)
              .foregroundColor(Color(braveSystemName: .primitivePrimary80))
                + Text(
                  product.price.frontSymbolCurrencyFormatted(
                    with: product.priceFormatStyle.locale,
                    isSymbolIncluded: false
                  ) ?? product.displayPrice
                )
                .font(.title)
                .foregroundColor(.white)
                + Text(
                  " / \(type == .monthly ? Strings.AIChat.paywallMonthlyPriceDividend : Strings.AIChat.paywallYearlyPriceDividend)"
                )
                .font(.subheadline)
                .foregroundColor(Color(braveSystemName: .primitivePrimary80))
            }
          } else {
            ProgressView()
              .tint(Color.white)
          }
        }
        .padding()
        .background(
          Color(
            braveSystemName: selectedTierType == type ? .primitivePrimary40 : .primitivePrimary20
          )
        )
        .overlay(
          ContainerRelativeShape()
            .strokeBorder(
              Color(braveSystemName: .primitivePrimary50),
              lineWidth: selectedTierType == type ? 2.0 : 0.0
            )
        )
        .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
      }
    )
    .frame(maxWidth: .infinity)
    .buttonStyle(.plain)
  }
}

private struct AIChatActionButton: View {
  var title: String
  var label: String
  var action: () -> Void

  var body: some View {
    VStack(spacing: 8) {
      Text(title)
        .font(.callout.weight(.semibold))
        .foregroundColor(Color(braveSystemName: .primitivePrimary90))
        .multilineTextAlignment(.center)

      Button(
        action: action,
        label: {
          HStack {
            Text(label)
              .font(.subheadline.weight(.semibold))
              .foregroundColor(Color(.white))
              .padding()
          }
          .frame(maxWidth: .infinity)
          .contentShape(ContainerRelativeShape())
        }
      )
      .overlay(
        ContainerRelativeShape()
          .strokeBorder(
            Color(braveSystemName: .dividerInteractive),
            lineWidth: 1.0
          )
      )
      .containerShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
    }
    .padding([.horizontal], 16.0)
  }
}

#if DEBUG
#Preview("External Purchase") {
  AIChatPaywallView()
    .environment(\.allowExternalPurchaseLinks, true)
}
#Preview("Standard Purchase") {
  AIChatPaywallView()
    .environment(\.allowExternalPurchaseLinks, false)
}

#Preview("LeoPremiumTier") {
  VStack {
    AIChatPremiumTierSelectionView(
      title: "Monthly",
      description: nil,
      product: nil,
      type: .yearly,
      selectedTierType: Binding.constant(.yearly)
    )

    AIChatPremiumTierSelectionView(
      title: "Yearly",
      description: "%17 Discount",
      product: nil,
      type: .monthly,
      selectedTierType: Binding.constant(.yearly)
    )

  }
}
#endif
