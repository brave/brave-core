// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import SwiftUI

struct SaferSignMessageRequestContainerView: View {

  let account: BraveWallet.AccountInfo
  let request: BraveWallet.SignMessageRequest
  let network: BraveWallet.NetworkInfo?
  let requestIndex: Int
  let requestCount: Int

  let namedFromAddress: String
  let receiverAddress: String
  let namedReceiverAddress: String
  let cowSwapOrder: BraveWallet.CowSwapOrder
  let ethSwapDetails: EthSwapDetails?

  /// A map between request id and a boolean value indicates this request message needs pilcrow formating.
  @Binding var needPilcrowFormatted: [Int32: Bool]
  /// A map between request id and a boolean value indicates this request message is displayed as
  /// its original content.
  @Binding var showOrignalMessage: [Int32: Bool]

  var nextTapped: () -> Void
  var action: (_ approved: Bool) -> Void

  @State private var isShowingDetails: Bool = false

  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.pixelLength) private var pixelLength
  @ScaledMetric private var faviconSize = 48
  private let maxFaviconSize: CGFloat = 72
  @ScaledMetric private var assetNetworkIconSize: CGFloat = 15
  private let maxAssetNetworkIconSize: CGFloat = 20

  var body: some View {
    ScrollView {
      VStack {
        requestsHeader

        originAndFavicon

        Spacer(minLength: 20)

        if isShowingDetails {
          SignMessageRequestContentView(
            request: request,
            needPilcrowFormatted: $needPilcrowFormatted,
            showOrignalMessage: $showOrignalMessage
          )
          // match spacing from comparison in `SaferSignTransactionView`
          .padding(.vertical, 20)
        } else {
          SaferSignTransactionView(
            network: network,
            fromAddress: account.address,
            namedFromAddress: namedFromAddress,
            receiverAddress: receiverAddress,
            namedReceiverAddress: namedReceiverAddress,
            fromToken: ethSwapDetails?.fromToken,
            fromTokenContractAddress: cowSwapOrder.sellToken,
            fromAmount: ethSwapDetails?.fromAmount,
            toToken: ethSwapDetails?.toToken,
            toTokenContractAddress: cowSwapOrder.buyToken,
            minBuyAmount: ethSwapDetails?.minBuyAmount
          )

        }

        networkFeeSection

        buttonsContainer
          .padding(.top)
          .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
          .accessibility(hidden: sizeCategory.isAccessibilityCategory)
      }
      .padding()
    }
    .overlay(alignment: .bottom) {
      if sizeCategory.isAccessibilityCategory {
        buttonsContainer
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
    }
    .navigationTitle(Strings.Wallet.swapConfirmationTitle)
    .navigationBarTitleDisplayMode(.inline)
  }

  /// Header containing the current requests network chain name, and a `1 of N` & `Next` button when there are multiple requests.
  private var requestsHeader: some View {
    HStack {
      if let network {
        Text(network.chainName)
          .font(.callout)
          .foregroundColor(Color(.braveLabel))
      }
      Spacer()
      if requestCount > 1 {
        NextIndexButton(
          currentIndex: requestIndex,
          count: requestCount,
          nextTapped: nextTapped
        )
      }
    }
  }

  private var originAndFavicon: some View {
    VStack {
      Group {
        if request.originInfo.isBraveWalletOrigin {
          Image(uiImage: UIImage(sharedNamed: "brave.logo")!)
            .resizable()
            .aspectRatio(contentMode: .fit)
            .foregroundColor(Color(.braveOrange))
        } else {
          if let url = URL(string: request.originInfo.originSpec) {
            FaviconReader(url: url) { image in
              if let image = image {
                Image(uiImage: image)
                  .resizable()
                  .scaledToFit()
              } else {
                Circle()
                  .stroke(Color(.braveSeparator), lineWidth: pixelLength)
              }
            }
            .background(Color(.braveDisabled))
            .clipShape(RoundedRectangle(cornerRadius: 4))
          }
        }
      }
      .frame(width: min(faviconSize, maxFaviconSize), height: min(faviconSize, maxFaviconSize))

      Text(originInfo: request.originInfo)
        .foregroundColor(Color(.braveLabel))
        .font(.subheadline)
        .multilineTextAlignment(.center)
        .padding(.top, 8)
    }
  }

  private var networkFeeSection: some View {
    VStack(alignment: .leading, spacing: 4) {
      HStack(alignment: .top) {
        VStack(alignment: .leading, spacing: 4) {
          Text(Strings.Wallet.swapConfirmationNetworkFee)
            .fontWeight(.medium)
            .foregroundColor(Color(.secondaryBraveLabel))
          HStack {
            Group {
              if let network {
                NetworkIcon(network: network)
              } else {
                Circle()
                  .stroke(Color(.braveSeparator))
                  .background(Color(braveSystemName: .iconInteractive).clipShape(Circle()))
              }
            }
            .frame(
              width: min(assetNetworkIconSize, maxAssetNetworkIconSize),
              height: min(assetNetworkIconSize, maxAssetNetworkIconSize)
            )
            Text(Strings.Wallet.braveSwapFree)
              .foregroundColor(Color(.braveLabel))
          }
          .frame(maxWidth: .infinity, alignment: .leading)
        }
        Spacer()
        Button {
          isShowingDetails.toggle()
        } label: {
          Text(detailsButtonTitle)
            .fontWeight(.medium)
            .foregroundColor(Color(braveSystemName: .textInteractive))
        }
        .transaction {
          $0.disablesAnimations = true
        }
      }
    }
    .frame(maxWidth: .infinity)
  }

  private var detailsButtonTitle: String {
    if isShowingDetails {
      return Strings.Wallet.hideDetailsButtonTitle
    }
    return Strings.Wallet.confirmationViewModeDetails
  }

  /// Cancel & Sign button container
  @ViewBuilder private var buttonsContainer: some View {
    if sizeCategory.isAccessibilityCategory {
      VStack {
        buttons
      }
    } else {
      HStack {
        buttons
      }
    }
  }

  /// Cancel and Sign buttons
  @ViewBuilder private var buttons: some View {
    Button {  // cancel
      action(false)
    } label: {
      Label(Strings.cancelButtonTitle, systemImage: "xmark")
        .imageScale(.large)
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    .disabled(requestIndex != 0)
    Button {  // approve
      action(true)
    } label: {
      Label(Strings.Wallet.sign, braveSystemImage: "leo.key")
        .imageScale(.large)
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
    .disabled(requestIndex != 0)
  }
}
