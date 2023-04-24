// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import BigNumber
import Strings
import DesignSystem

struct SwapTransactionConfirmationView: View {
  
  let parsedTransaction: ParsedTransaction?
  let network: BraveWallet.NetworkInfo?
  
  let editGasFeeTapped: () -> Void
  let advancedSettingsTapped: () -> Void
  
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.colorScheme) private var colorScheme
  @Environment(\.pixelLength) private var pixelLength
  @ScaledMetric private var faviconSize = 48
  private let maxFaviconSize: CGFloat = 72
  @ScaledMetric private var assetIconSize: CGFloat = 40
  private let maxAssetIconSize: CGFloat = 50
  @ScaledMetric private var assetNetworkIconSize: CGFloat = 15
  private let maxAssetNetworkIconSize: CGFloat = 20
  
  var body: some View {
    VStack(spacing: 20) {
      originAndFavicon
      
      tokenValueComparison

      tokensView

      networkFeeView
    }
    .padding(.bottom, 20)
  }
  
  private var originAndFavicon: some View {
    VStack {
      if let originInfo = parsedTransaction?.transaction.originInfo {
        Group {
          if originInfo.origin == WalletConstants.braveWalletOrigin {
            Image(uiImage: UIImage(sharedNamed: "brave.logo")!)
              .resizable()
              .aspectRatio(contentMode: .fit)
              .foregroundColor(Color(.braveOrange))
          } else {
            originInfo.origin.url.map { url in
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
        Text(urlOrigin: originInfo.origin)
          .foregroundColor(Color(.braveLabel))
          .font(.subheadline)
          .multilineTextAlignment(.center)
          .padding(.top, 8)
      }
    }
  }
  
  @ViewBuilder private var tokenValueComparison: some View {
    if let minBuyAmount = Double(parsedTransaction?.ethSwap?.minBuyAmount ?? ""),
       let sellAmount = Double( parsedTransaction?.ethSwap?.fromAmount ?? ""),
       minBuyAmount != 0, sellAmount != 0 {
      let calculated = minBuyAmount / sellAmount
      let display = String(format: "1 \(parsedTransaction?.ethSwap?.fromToken?.symbol ?? "") = %.6f \(parsedTransaction?.ethSwap?.toToken?.symbol ?? "")", calculated)
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
        AddressView(address: parsedTransaction?.fromAddress ?? "") {
          HStack(spacing: 2) {
            Blockie(address: parsedTransaction?.fromAddress ?? "")
              .frame(width: 15, height: 15)
            Text(parsedTransaction?.namedFromAddress ?? "")
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
        title: "\(parsedTransaction?.ethSwap?.fromAmount ?? "") \(parsedTransaction?.ethSwap?.fromToken?.symbol ?? "")",
        subTitle: String.localizedStringWithFormat(Strings.Wallet.swapConfirmationNetworkDesc, network?.chainName ?? ""),
        token: parsedTransaction?.ethSwap?.fromToken,
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
      }
      TokenRow(
        title: "\(parsedTransaction?.ethSwap?.minBuyAmount ?? "") \(parsedTransaction?.ethSwap?.toToken?.symbol ?? "")",
        subTitle: String.localizedStringWithFormat(Strings.Wallet.swapConfirmationNetworkDesc, network?.chainName ?? ""),
        token: parsedTransaction?.ethSwap?.toToken,
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
  
  private var networkFeeView: some View {
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
          }
        }
        .frame(width: min(assetNetworkIconSize, maxAssetNetworkIconSize), height: min(assetNetworkIconSize, maxAssetNetworkIconSize))
        Text(parsedTransaction?.gasFee?.fiat ?? "")
          .foregroundColor(Color(.braveLabel))
        Button(action: editGasFeeTapped) {
          Text(Strings.Wallet.editGasFeeButtonTitle)
            .fontWeight(.semibold)
            .foregroundColor(Color(.braveBlurpleTint))
        }
        Spacer()
      }
    }
    .frame(maxWidth: .infinity)
  }
  
  private struct TokenRow: View {
    let title: String
    let subTitle: String
    let token: BraveWallet.BlockchainToken?
    let network: BraveWallet.NetworkInfo?
    let assetIconSize: CGFloat
    let maxAssetIconSize: CGFloat
    let assetNetworkIconSize: CGFloat
    let maxAssetNetworkIconSize: CGFloat
    
    var body: some View {
      HStack {
        if let token = token, let network = network {
          AssetIconView(
            token: token,
            network: network,
            shouldShowNativeTokenIcon: true,
            length: assetIconSize,
            maxLength: maxAssetIconSize,
            networkSymbolLength: assetNetworkIconSize,
            maxNetworkSymbolLength: maxAssetNetworkIconSize
          )
        } else {
          Circle()
            .stroke(Color(.braveSeparator))
            .frame(width: assetIconSize, height: assetIconSize)
        }
        VStack(alignment: .leading) {
          Text(title)
            .font(.callout)
            .fontWeight(.semibold)
          Text(subTitle)
            .font(.footnote)
        }
        Spacer()
      }
    }
  }
}

#if DEBUG
struct SwapTransactionConfirmationView_Previews: PreviewProvider {
  static var transaction: BraveWallet.TransactionInfo {
    let transaction: BraveWallet.TransactionInfo = .init()
    transaction.originInfo = .init(
      origin: WalletConstants.braveWalletOrigin,
      originSpec: "brave://wallet",
      eTldPlusOne: ""
    )
    return transaction
  }
  
  static var previews: some View {
    let parsedTransaction: ParsedTransaction = .init(
      transaction: transaction,
      namedFromAddress: "Ethereum Account 1",
      fromAddress: BraveWallet.AccountInfo.previewAccount.address,
      namedToAddress: "0x Exchange",
      toAddress: "0x1111111111222222222233333333334444444444",
      networkSymbol: "ETH",
      details: .ethSwap(.init(
        fromToken: .mockUSDCToken,
        fromValue: "1.000004",
        fromAmount: "1",
        fromFiat: "$1.04",
        toToken: .previewDaiToken,
        minBuyValue: "0.994798",
        minBuyAmount: "0.994798",
        minBuyAmountFiat: "$0.99",
        gasFee: .init(fee: "100", fiat: "$0.009081")
      ))
    )
    Group {
      ForEach(ColorScheme.allCases, id: \.self) { colorScheme in
        ScrollView {
          SwapTransactionConfirmationView(
            parsedTransaction: parsedTransaction,
            network: .mockPolygon,
            editGasFeeTapped: {},
            advancedSettingsTapped: {}
          )
          Spacer()
        }
        .background(Color(.braveGroupedBackground).ignoresSafeArea())
        .preferredColorScheme(colorScheme)
      }
    }
  }
}
#endif
