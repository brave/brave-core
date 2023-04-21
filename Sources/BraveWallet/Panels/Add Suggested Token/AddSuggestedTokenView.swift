// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Strings
import BraveCore
import DesignSystem

struct AddSuggestedTokenView: View {
  var token: BraveWallet.BlockchainToken
  var originInfo: BraveWallet.OriginInfo
  var cryptoStore: CryptoStore
  var onDismiss: () -> Void
  
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.openURL) private var openWalletURL
  
  var body: some View {
    ScrollView(.vertical) {
      VStack(spacing: 22) {
        VStack(spacing: 8) {
          Text(Strings.Wallet.addSuggestedTokenSubtitle)
            .font(.headline)
            .foregroundColor(Color(.bravePrimary))
          Text(urlOrigin: originInfo.origin)
            .font(.footnote)
            .foregroundColor(Color(.braveLabel))
        }
        .padding(.top)
        VStack {
          VStack {
            AssetIconView(token: token, network: cryptoStore.networkStore.selectedChain, length: 64)
            Text(token.symbol)
              .font(.headline)
              .foregroundColor(Color(.bravePrimary))
          }
          .accessibilityElement(children: .combine)
          Button(action: {
            if let baseURL = cryptoStore.networkStore.selectedChain.blockExplorerUrls.first.map(URL.init(string:)),
               let url = baseURL?.appendingPathComponent("token/\(token.contractAddress)") {
              openWalletURL(url)
            }
          }) {
            HStack {
              Text(token.contractAddress.truncatedAddress)
              Image(systemName: "arrow.up.forward.square")
            }
            .font(.subheadline)
            .foregroundColor(Color(.braveBlurpleTint))
          }
          .accessibilityLabel(Strings.Wallet.contractAddressAccessibilityLabel)
          .accessibilityValue(token.contractAddress.truncatedAddress)
        }
        actionButtonContainer
          .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
          .accessibility(hidden: sizeCategory.isAccessibilityCategory)
          .padding(.top, 20)
      }
      .frame(maxWidth: .infinity)
      .padding()
    }
    .background(Color(.braveGroupedBackground).ignoresSafeArea())
    .navigationTitle(Strings.Wallet.addSuggestedTokenTitle)
    .navigationBarTitleDisplayMode(.inline)
    .overlay(
      Group {
        if sizeCategory.isAccessibilityCategory {
          actionButtonContainer
            .frame(maxWidth: .infinity)
            .padding(.top)
            .background(
              LinearGradient(
                stops: [
                  .init(color: Color(.braveGroupedBackground).opacity(0), location: 0),
                  .init(color: Color(.braveGroupedBackground).opacity(1), location: 0.05),
                  .init(color: Color(.braveGroupedBackground).opacity(1), location: 1),
                ],
                startPoint: .top,
                endPoint: .bottom
              )
                .ignoresSafeArea()
                .allowsHitTesting(false)
            )
        }
      },
      alignment: .bottom
    )
  }
  
  @ViewBuilder private var actionButtonContainer: some View {
    if sizeCategory.isAccessibilityCategory {
      VStack {
        actionButtons
      }
    } else {
      HStack {
        actionButtons
      }
    }
  }

  @ViewBuilder private var actionButtons: some View {
    Button(action: { // cancel
      cryptoStore.handleWebpageRequestResponse(
        .addSuggestedToken(approved: false, contractAddresses: [token.contractAddress])
      )
      onDismiss()
    }) {
      HStack {
        Image(systemName: "xmark")
        Text(Strings.cancelButtonTitle)
      }
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    Button(action: { // approve
      cryptoStore.handleWebpageRequestResponse(
        .addSuggestedToken(approved: true, contractAddresses: [token.contractAddress])
      )
      onDismiss()
    }) {
      HStack {
        Image(braveSystemName: "leo.check.circle-filled")
        Text(Strings.Wallet.add)
          .multilineTextAlignment(.center)
      }
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
  }
}

#if DEBUG
struct AddSuggestedTokenView_Previews: PreviewProvider {
  static var previews: some View {
    AddSuggestedTokenView(
      token: .previewToken,
      originInfo: .init(
        origin: .init(url: URL(string: "https://app.uniswap.org")!),
        originSpec: "",
        eTldPlusOne: "uniswap.org"
      ),
      cryptoStore: .previewStore,
      onDismiss: { }
    )
  }
}
#endif
