// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import DesignSystem
import Strings
import SwiftUI

struct SaferSignTransactionContainerView: View {
  /// The OriginInfo that created the transaction
  let originInfo: BraveWallet.OriginInfo?
  /// The network the transaction belongs to
  let network: BraveWallet.NetworkInfo?

  /// The address of the account making the swap
  let fromAddress: String?
  /// The name of the account
  let namedFromAddress: String?

  /// The token being swapped from.
  let fromToken: BraveWallet.BlockchainToken?
  /// The amount of the `tokToken` being swapped.
  let fromAmount: String?

  /// The token being swapped for.
  let toToken: BraveWallet.BlockchainToken?
  /// Minimum amount being bought of the `toToken`.
  let minBuyAmount: String?
  /// The gas fee for the transaction
  let gasFee: GasFee?

  let editGasFeeTapped: () -> Void
  let advancedSettingsTapped: () -> Void

  @Environment(\.pixelLength) private var pixelLength
  @ScaledMetric private var faviconSize = 48
  private let maxFaviconSize: CGFloat = 72
  @ScaledMetric private var assetNetworkIconSize: CGFloat = 15
  private let maxAssetNetworkIconSize: CGFloat = 20

  init(
    parsedTransaction: ParsedTransaction,
    editGasFeeTapped: @escaping () -> Void,
    advancedSettingsTapped: @escaping () -> Void
  ) {
    self.originInfo = parsedTransaction.transaction.originInfo
    self.network = parsedTransaction.network
    self.fromAddress = parsedTransaction.fromAccountInfo.address
    self.namedFromAddress = parsedTransaction.namedFromAddress
    if case .ethSwap(let details) = parsedTransaction.details {
      self.fromToken = details.fromToken
      self.fromAmount = details.fromAmount
      self.toToken = details.toToken
      self.minBuyAmount = details.minBuyAmount
    } else {
      self.fromToken = nil
      self.fromAmount = nil
      self.toToken = nil
      self.minBuyAmount = nil
    }
    self.gasFee = parsedTransaction.gasFee
    self.editGasFeeTapped = editGasFeeTapped
    self.advancedSettingsTapped = advancedSettingsTapped
  }

  var body: some View {
    VStack {
      originAndFavicon

      SaferSignTransactionView(
        network: network,
        fromAddress: fromAddress,
        namedFromAddress: namedFromAddress,
        receiverAddress: nil,
        namedReceiverAddress: nil,
        fromToken: fromToken,
        fromTokenContractAddress: fromToken?.contractAddress,
        fromAmount: fromAmount,
        toToken: toToken,
        toTokenContractAddress: toToken?.contractAddress,
        minBuyAmount: minBuyAmount
      )

      networkFeeSection
    }
  }

  private var originAndFavicon: some View {
    VStack {
      if let originInfo = originInfo {
        Group {
          if originInfo.isBraveWalletOrigin {
            Image(uiImage: UIImage(sharedNamed: "brave.logo")!)
              .resizable()
              .aspectRatio(contentMode: .fit)
              .foregroundColor(Color(.braveOrange))
          } else {
            if let url = URL(string: originInfo.originSpec) {
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

        Text(originInfo: originInfo)
          .foregroundColor(Color(.braveLabel))
          .font(.subheadline)
          .multilineTextAlignment(.center)
          .padding(.top, 8)
      }
    }
  }

  private var networkFeeSection: some View {
    VStack(alignment: .leading, spacing: 4) {
      HStack {
        Text(Strings.Wallet.swapConfirmationNetworkFee)
          .fontWeight(.medium)
          .foregroundColor(Color(.secondaryBraveLabel))
        Spacer()
        Button(action: advancedSettingsTapped) {
          Image(systemName: "gearshape")
            .foregroundColor(Color(.secondaryBraveLabel))
        }
        .buttonStyle(.plain)
      }
      HStack {
        Group {
          if let image = network?.nativeTokenLogoImage {
            Image(uiImage: image)
              .resizable()
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
        Text(gasFee?.fiat ?? "")
          .foregroundColor(Color(.braveLabel))
        Button(action: editGasFeeTapped) {
          Text(Strings.Wallet.editButtonTitle)
            .fontWeight(.semibold)
            .foregroundColor(Color(.braveBlurpleTint))
        }
        Spacer()
      }
    }
    .frame(maxWidth: .infinity)
  }
}
