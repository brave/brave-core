// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStore
import BraveStrings
import DesignSystem
import Foundation
import Introspect
import StoreKit
import SwiftUI

public struct OriginPaywallView: View {
  @Bindable var viewModel: OriginPaywallViewModel

  @Environment(\.allowExternalPurchaseLinks) private var allowExternalPurchaseLinks
  @Environment(\.dismiss) private var dismiss
  @Environment(\.openURL) private var openURL

  public init(viewModel: OriginPaywallViewModel) {
    self.viewModel = viewModel
  }

  public var body: some View {
    NavigationStack {
      VStack(spacing: 0) {
        ScrollView {
          VStack(spacing: 16) {
            UpsellView()
            PaywallDivider()
              .padding(.horizontal, -16)
            ProductView(product: viewModel.product)
            // Paywall description has markdown
            Text(LocalizedStringKey(Strings.Origin.paywallDescription))
              .foregroundStyle(.white)
              .padding(.horizontal, 16)
              .padding(.vertical, 8)
              .font(.footnote)
            AdditionalActionView(
              title: Strings.Origin.alreadyPurchasedTitle
            ) {
              openURL(.braveOriginRefreshCredentials)
            } label: {
              Text(Strings.Origin.getLoginCodeButton)
            }
            AdditionalActionView(
              title: Strings.Origin.promoCodeTitle
            ) {
              // Open the redeem code sheet
              SKPaymentQueue.default().presentCodeRedemptionSheet()
            } label: {
              Text(Strings.Origin.redeemPromoCodeButton)
            }
          }
          .frame(maxWidth: .infinity)
          .padding(16)
        }
        paywallActionContainerView
      }
      .background(Color(braveSystemName: .primitiveBlurple10))
      .navigationTitle(Strings.Origin.originProductName)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Group {
            if #available(iOS 26.0, *) {
              Button(role: .close) {
                dismiss()
              }
            } else {
              Button(Strings.close) {
                dismiss()
              }
            }
          }
          .tint(.white)
        }
        ToolbarItemGroup(placement: .topBarTrailing) {
          Button(Strings.Origin.restoreButton) {
            Task {
              await viewModel.restore()
            }
          }
          .tint(.white)
        }
      }
      .alert(Strings.genericErrorTitle, isPresented: $viewModel.isErrorPresented) {
        Button(Strings.OKString) {
        }
      } message: {
        Text(Strings.Origin.purchaseErrorMessage)
      }
      .introspectViewController { vc in
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
      }
    }
    .colorScheme(.dark)
    .preferredColorScheme(.dark)
  }

  private var standardPaywallActionView: some View {
    Button {
      Task {
        await viewModel.purchase()
      }
    } label: {
      HStack {
        if viewModel.isStoreOperationActive {
          ProgressView()
            .tint(Color.white)
            .padding()
        } else {
          Text(Strings.Origin.buyNowButton)
            .font(.headline)
            .foregroundColor(Color(.white))
            .padding()
        }
      }
      .frame(maxWidth: .infinity)
      .background(
        LinearGradient(
          colors: [
            Color(braveSystemName: .primitiveBrandsRorange2),
            Color(braveSystemName: .primitiveBrandsRorange3),
          ],
          startPoint: .top,
          endPoint: .bottom
        ).shadow(.inner(color: .white.opacity(0.3), radius: 1, y: 2)),
        in: .capsule
      )
      .overlay {
        Capsule()
          .strokeBorder(Color(braveSystemName: .primitiveBrandsRorange2))
      }
    }
    .disabled(viewModel.isStoreOperationActive)
    .padding(16)
    .overlay(alignment: .top) {
      PaywallDivider()
    }
  }

  private var externalPurchasesAllowedActionView: some View {
    VStack(spacing: 16) {
      VStack(spacing: 0) {
        Text(Strings.Paywall.startTrialSubtitle)
          .font(.footnote)
      }
      .multilineTextAlignment(.center)
      VStack {
        Button {
          Task {
            await viewModel.purchase()
          }
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
          .opacity(viewModel.isStoreOperationActive ? 0 : 1)
          .overlay {
            if viewModel.isStoreOperationActive {
              ProgressView()
                .tint(Color.white)
                .padding()
            }
          }
          .frame(maxWidth: .infinity)
          .padding(.horizontal, 16)
          .padding(.vertical, 12)
          .frame(minHeight: 52)
          .background(
            Color(braveSystemName: .primitiveBlurple35),
            in: .capsule
          )
        }
        .disabled(viewModel.isStoreOperationActive)
        Button {
          openURL(.braveOriginCheckoutURL)
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
          .frame(maxWidth: .infinity)
          .padding(.horizontal, 16)
          .padding(.vertical, 12)
          .frame(minHeight: 52)
          .background(
            LinearGradient(
              colors: [
                Color(braveSystemName: .primitiveBrandsRorange2),
                Color(braveSystemName: .primitiveBrandsRorange3),
              ],
              startPoint: .top,
              endPoint: .bottom
            ).shadow(.inner(color: .white.opacity(0.3), radius: 1, y: 2)),
            in: .capsule
          )
          .overlay {
            Capsule()
              .strokeBorder(Color(braveSystemName: .primitiveBrandsRorange2))
          }
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

  @ViewBuilder private var paywallActionContainerView: some View {
    if allowExternalPurchaseLinks {
      externalPurchasesAllowedActionView
    } else {
      standardPaywallActionView
    }
  }

  struct PaywallDivider: View {
    @Environment(\.pixelLength) private var pixelLength
    var body: some View {
      Color(braveSystemName: .primitivePrimary25)
        .frame(height: pixelLength)
    }
  }

  struct UpsellView: View {
    let upsells: [String] = [
      Strings.Origin.upsellSupportMission,
      Strings.Origin.upsellMinimalUI,
      Strings.Origin.upsellCoreFeatures,
      Strings.Origin.upsellOneTimePurchase,
    ]

    var body: some View {
      VStack(spacing: 8) {
        ForEach(upsells, id: \.self) { upsell in
          Label {
            Text(upsell)
              .foregroundStyle(.white)
          } icon: {
            Image(braveSystemName: "leo.check.normal")
              .foregroundStyle(LinearGradient(braveSystemName: .toolbarBackground))
          }
          .frame(maxWidth: .infinity, alignment: .leading)
        }
      }
    }
  }

  struct ProductView: View {
    var product: Product?

    var body: some View {
      HStack {
        Text(Strings.Origin.oneTimePurchaseLabel)
          .fontWeight(.semibold)
        Spacer()
        if let product {
          let currencyIdentifier = product.priceFormatStyle.locale.currency?.identifier ?? ""
          let currencySymbol = product.priceFormatStyle.locale.currencySymbol ?? ""
          let currencyCodeText = Text("\(currencyIdentifier)\(currencySymbol)")
            .font(.footnote)
          let productPriceText = Text(
            product.price,
            format: .currency(code: "").locale(product.priceFormatStyle.locale)
          ).fontWeight(.semibold)
          Text("\(currencyCodeText)\(productPriceText)")
            .foregroundStyle(.white)
        } else {
          ProgressView()
            .tint(Color.white)
            .progressViewStyle(.circular)
        }
      }
      .foregroundStyle(.white)
      .padding(16)
      .background(Color(braveSystemName: .primitiveBlurple35), in: .containerRelative)
      .overlay {
        ContainerRelativeShape()
          .strokeBorder(Color(braveSystemName: .primitiveBlurple50), style: .init(lineWidth: 2))
      }
      .containerShape(.rect(cornerRadius: 12, style: .continuous))
    }
  }

  /// A view which displays a title centered above a button with standard styling, typically
  /// placed at the bottom of the paywall
  struct AdditionalActionView<Title: View, Label: View>: View {
    var title: Title
    var action: () -> Void
    var label: Label

    init<S: StringProtocol>(
      title: S,
      action: @escaping () -> Void,
      @ViewBuilder label: () -> Label
    ) where Title == Text {
      self.title = Text(title)
      self.label = label()
      self.action = action
    }

    var body: some View {
      VStack(spacing: 8) {
        title
          .font(.subheadline)
          .foregroundColor(Color(braveSystemName: .primitiveBlurple98))
        Button(
          action: action,
          label: {
            label
              .font(.subheadline.weight(.semibold))
              .foregroundStyle(Color(braveSystemName: .textInteractive))
              .padding()
              .frame(maxWidth: .infinity)
              .overlay(
                Capsule()
                  .strokeBorder(Color(braveSystemName: .dividerInteractive), lineWidth: 1)
              )
          }
        )
      }
    }
  }
}

#if DEBUG
#Preview("External Purchases") {
  OriginPaywallView(viewModel: .init(store: .init(skusService: nil)))
    .environment(\.allowExternalPurchaseLinks, true)
}
#Preview("Standard Purchases") {
  OriginPaywallView(viewModel: .init(store: .init(skusService: nil)))
    .environment(\.allowExternalPurchaseLinks, false)
}
#Preview("Sheet Presented") {
  Color.white
    .sheet(isPresented: .constant(true)) {
      OriginPaywallView(viewModel: .init(store: .init(skusService: nil)))
    }
}
#endif
