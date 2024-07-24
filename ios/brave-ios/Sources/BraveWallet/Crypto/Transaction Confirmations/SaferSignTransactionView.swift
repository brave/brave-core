// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import DesignSystem
import Strings
import SwiftUI

struct SaferSignTransactionView: View {
  /// The network the transaction belongs to
  let network: BraveWallet.NetworkInfo?

  /// The address of the account making the swap
  let fromAddress: String?
  /// The name of the account
  let namedFromAddress: String?

  /// The address of the recipient (applicable to CoW Swap)
  let receiverAddress: String?
  /// The named address of the recipient (applicable to CoW Swap)
  let namedReceiverAddress: String?

  /// The token being swapped from.
  let fromToken: BraveWallet.BlockchainToken?
  /// The contract address of the from token
  let fromTokenContractAddress: String?
  /// The amount of the `tokToken` being swapped.
  let fromAmount: String?

  /// The token being swapped for.
  let toToken: BraveWallet.BlockchainToken?
  /// The contract address of the to token
  let toTokenContractAddress: String?
  /// Minimum amount being bought of the `toToken`.
  let minBuyAmount: String?

  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.colorScheme) private var colorScheme
  @Environment(\.pixelLength) private var pixelLength
  @ScaledMetric private var assetIconSize: CGFloat = 40
  private let maxAssetIconSize: CGFloat = 50
  @ScaledMetric private var assetNetworkIconSize: CGFloat = 15
  private let maxAssetNetworkIconSize: CGFloat = 20

  init(
    network: BraveWallet.NetworkInfo?,
    fromAddress: String?,
    namedFromAddress: String?,
    receiverAddress: String?,
    namedReceiverAddress: String?,
    fromToken: BraveWallet.BlockchainToken?,
    fromTokenContractAddress: String?,
    fromAmount: String?,
    toToken: BraveWallet.BlockchainToken?,
    toTokenContractAddress: String?,
    minBuyAmount: String?
  ) {
    self.network = network
    self.fromAddress = fromAddress
    self.namedFromAddress = namedFromAddress
    self.receiverAddress = receiverAddress
    self.namedReceiverAddress = namedReceiverAddress
    self.fromToken = fromToken
    self.fromTokenContractAddress = fromTokenContractAddress
    self.fromAmount = fromAmount
    self.toToken = toToken
    self.toTokenContractAddress = toTokenContractAddress
    self.minBuyAmount = minBuyAmount
  }

  var body: some View {
    VStack(spacing: 20) {
      tokenValueComparison

      tokensView
    }
    .padding(.bottom, 20)
  }

  @ViewBuilder private var tokenValueComparison: some View {
    if let minBuyAmount = Double(minBuyAmount ?? ""),
      let sellAmount = Double(fromAmount ?? ""),
      minBuyAmount != 0, sellAmount != 0
    {
      let calculated = minBuyAmount / sellAmount
      let display = String(
        format: "1 \(fromToken?.symbol ?? "") = %.6f \(toToken?.symbol ?? "")",
        calculated
      )
      Text(display)
        .font(.callout)
        .foregroundColor(Color(.secondaryBraveLabel))
    }
  }

  private var fromTokenView: some View {
    VStack {
      HStack {
        Text(Strings.Wallet.swapConfirmationYouSpend)
          .fontWeight(.medium)
          .foregroundColor(Color(.secondaryBraveLabel))
        Spacer()
        AddressView(address: fromAddress ?? "") {
          HStack(spacing: 2) {
            Blockie(address: fromAddress ?? "")
              .frame(width: 15, height: 15)
            Text(namedFromAddress ?? "")
              .font(.footnote)
          }
          .padding(4)
          .overlay(
            RoundedRectangle(cornerSize: CGSize(width: 4, height: 4))
              .stroke(Color(.braveSeparator), lineWidth: pixelLength)
          )
        }
      }
      TokenRow(
        title: "\(fromAmount ?? "") \(fromToken?.symbol ?? "")",
        subTitle: String.localizedStringWithFormat(
          Strings.Wallet.swapConfirmationNetworkDesc,
          network?.chainName ?? ""
        ),
        token: fromToken,
        tokenContractAddress: fromTokenContractAddress,
        network: network,
        assetIconSize: assetIconSize,
        maxAssetIconSize: maxAssetIconSize,
        assetNetworkIconSize: assetNetworkIconSize,
        maxAssetNetworkIconSize: maxAssetNetworkIconSize
      )
    }
  }

  private var toTokenView: some View {
    VStack {
      HStack {
        Text(Strings.Wallet.swapConfirmationYoullReceive)
          .fontWeight(.medium)
          .foregroundColor(Color(.secondaryBraveLabel))
        Spacer()
        if let receiverAddress {
          AddressView(address: receiverAddress) {
            HStack(spacing: 2) {
              Blockie(address: receiverAddress)
                .frame(width: 15, height: 15)
              Text(namedReceiverAddress ?? receiverAddress.truncatedAddress)
                .font(.footnote)
            }
            .padding(4)
            .overlay(
              RoundedRectangle(cornerSize: CGSize(width: 4, height: 4))
                .stroke(Color(.braveSeparator), lineWidth: pixelLength)
            )
          }
        }
      }
      TokenRow(
        title: "\(minBuyAmount ?? "") \(toToken?.symbol ?? "")",
        subTitle: String.localizedStringWithFormat(
          Strings.Wallet.swapConfirmationNetworkDesc,
          network?.chainName ?? ""
        ),
        token: toToken,
        tokenContractAddress: toTokenContractAddress,
        network: network,
        assetIconSize: assetIconSize,
        maxAssetIconSize: maxAssetIconSize,
        assetNetworkIconSize: assetNetworkIconSize,
        maxAssetNetworkIconSize: maxAssetNetworkIconSize
      )
    }
  }

  private var arrowView: some View {
    Circle()
      .stroke(Color(.braveSeparator), lineWidth: pixelLength)
      .background(Color(.secondaryBraveGroupedBackground))
      .frame(width: 32, height: 32)
      .overlay(
        Image(systemName: "arrow.down")
          .resizable()
          .aspectRatio(contentMode: .fit)
          .frame(width: 16, height: 16)
          .foregroundColor(Color(white: 0.75))
      )
  }

  private var tokensView: some View {
    VStack {
      fromTokenView
        .padding(.init(top: 10, leading: 15, bottom: 10, trailing: 10))
      Divider()
        .overlay(arrowView, alignment: .center)
        .padding(.vertical, 5)
      toTokenView
        .padding(.init(top: 10, leading: 15, bottom: 20, trailing: 10))
    }
    .background(Color(.secondaryBraveGroupedBackground).cornerRadius(10))
  }
}

private struct TokenRow: View {
  let title: String
  let subTitle: String
  let token: BraveWallet.BlockchainToken?
  let tokenContractAddress: String?
  let network: BraveWallet.NetworkInfo?
  let assetIconSize: CGFloat
  let maxAssetIconSize: CGFloat
  let assetNetworkIconSize: CGFloat
  let maxAssetNetworkIconSize: CGFloat

  @Environment(\.openURL) private var openWalletURL

  var body: some View {
    HStack {
      if let token = token, let network = network {
        AssetIconView(
          token: token,
          network: network,
          shouldShowNetworkIcon: true,
          length: assetIconSize,
          maxLength: maxAssetIconSize,
          networkSymbolLength: assetNetworkIconSize,
          maxNetworkSymbolLength: maxAssetNetworkIconSize
        )
      } else {
        Circle()
          .stroke(Color(.braveSeparator))
          .frame(width: assetIconSize, height: assetIconSize)
          .background(Color(braveSystemName: .iconInteractive).clipShape(Circle()))
      }
      VStack(alignment: .leading) {
        Text(title)
          .font(.callout)
          .fontWeight(.semibold)
        Text(subTitle)
          .font(.footnote)
      }
      Spacer()
      if let toTokenContractAddress = tokenContractAddress ?? token?.contractAddress,
        let explorerUrl = network?.tokenBlockExplorerURL(toTokenContractAddress)
      {
        Button {
          openWalletURL(explorerUrl)
        } label: {
          Label(Strings.Wallet.viewOnBlockExplorer, systemImage: "arrow.up.forward.square")
            .labelStyle(.iconOnly)
            .foregroundColor(Color(braveSystemName: .iconInteractive))
        }
      }
    }
  }
}

#if DEBUG
struct SaferSignTransactionView_Previews: PreviewProvider {
  static var transaction: BraveWallet.TransactionInfo {
    let transaction: BraveWallet.TransactionInfo = .init()
    transaction.originInfo = .init(
      originSpec: WalletConstants.braveWalletOriginSpec,
      eTldPlusOne: ""
    )
    return transaction
  }

  static var previews: some View {
    let parsedTransaction: ParsedTransaction = .init(
      transaction: transaction,
      namedFromAddress: "Ethereum Account 1",
      fromAccountInfo: BraveWallet.AccountInfo.previewAccount,
      namedToAddress: "0x Exchange",
      toAddress: "0x1111111111222222222233333333334444444444",
      network: .mockMainnet,
      details: .ethSwap(
        .init(
          fromToken: .mockUSDCToken,
          fromNetwork: .mockMainnet,
          fromValue: "1.000004",
          fromAmount: "1",
          fromFiat: "$1.04",
          toToken: .previewDaiToken,
          toNetwork: .mockMainnet,
          minBuyValue: "0.994798",
          minBuyAmount: "0.994798",
          minBuyAmountFiat: "$0.99",
          gasFee: .init(fee: "100", fiat: "$0.009081")
        )
      )
    )
    Group {
      ForEach(ColorScheme.allCases, id: \.self) { colorScheme in
        ScrollView {
          SaferSignTransactionView(
            network: parsedTransaction.network,
            fromAddress: parsedTransaction.fromAccountInfo.address,
            namedFromAddress: parsedTransaction.namedFromAddress,
            receiverAddress: nil,
            namedReceiverAddress: nil,
            fromToken: .mockUSDCToken,
            fromTokenContractAddress: BraveWallet.BlockchainToken.mockUSDCToken.contractAddress,
            fromAmount: "1",
            toToken: .previewDaiToken,
            toTokenContractAddress: BraveWallet.BlockchainToken.previewDaiToken.contractAddress,
            minBuyAmount: "0.994798"
          )
        }
        .background(Color(.braveGroupedBackground).ignoresSafeArea())
        .preferredColorScheme(colorScheme)
      }
    }
  }
}
#endif
