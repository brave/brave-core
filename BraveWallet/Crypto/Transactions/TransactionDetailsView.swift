// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import BraveUI
import SwiftUI
import Swift
import Strings

struct TransactionDetailsView: View {
  
  var info: BraveWallet.TransactionInfo
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var keyringStore: KeyringStore
  var visibleTokens: [BraveWallet.BlockchainToken]
  var allTokens: [BraveWallet.BlockchainToken]
  var assetRatios: [String: Double]
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.openWalletURLAction) private var openWalletURL
  
  init(
    info: BraveWallet.TransactionInfo,
    networkStore: NetworkStore,
    keyringStore: KeyringStore,
    visibleTokens: [BraveWallet.BlockchainToken],
    allTokens: [BraveWallet.BlockchainToken],
    assetRatios: [String: Double],
    currencyCode: String
  ) {
    self.info = info
    self.networkStore = networkStore
    self.keyringStore = keyringStore
    self.visibleTokens = visibleTokens
    self.allTokens = allTokens
    self.assetRatios = assetRatios
    self.currencyFormatter.currencyCode = currencyCode
  }
  
  private let dateFormatter = DateFormatter().then {
    $0.dateFormat = "h:mm a - MMM d, yyyy"
  }
  
  private let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
  
  private func token(for contractAddress: String) -> BraveWallet.BlockchainToken? {
    let findToken: (BraveWallet.BlockchainToken) -> Bool = { $0.contractAddress.caseInsensitiveCompare(contractAddress) == .orderedSame }
    return visibleTokens.first(where: findToken) ?? allTokens.first(where: findToken)
  }

  /// The value of the transaction with symbol, ex. `"0.0100 ETH"`
  private var value: String {
    let formatter = WeiFormatter(decimalFormatStyle: .balance)
    let amount: String
    switch info.txType {
    case .erc20Transfer:
      amount = info.txArgs[safe: 0] ?? ""
    case .erc721TransferFrom, .erc721SafeTransferFrom:
      if let tokenID = info.txArgs[safe: 2],
         let tokenIDValue = Int(tokenID.removingHexPrefix, radix: 16),
         let contractAddress = info.txDataUnion.ethTxData1559?.baseData.to,
         let token = token(for: contractAddress) {
        return String(format: "%@ %d", token.name, tokenIDValue)
      } else {
        return ""
      }
    case .erc20Approve:
      if let contractAddress = info.txDataUnion.ethTxData1559?.baseData.to,
         let value = info.txArgs[safe: 1],
         let token = token(for: contractAddress) {
        if value.caseInsensitiveCompare(WalletConstants.MAX_UINT256) == .orderedSame {
          amount = Strings.Wallet.editPermissionsApproveUnlimited
        } else {
          amount = formatter.decimalString(for: value.removingHexPrefix, radix: .hex, decimals: Int(token.decimals)) ?? ""
        }
        return String(format: "%@ %@", amount, token.symbol)
      } else {
        return "0.0"
      }
    case .ethSend, .other:
      amount = formatter.decimalString(for: info.ethTxValue.removingHexPrefix, radix: .hex, decimals: Int(networkStore.selectedChain.decimals)) ?? ""
    @unknown default:
      amount = "0.0"
    }
    return String(format: "%@ %@", amount, networkStore.selectedChain.symbol)
  }
  
  /// The fiat of the transaction, ex. `"$45.76"`
  private var fiat: String? {
    let formatter = WeiFormatter(decimalFormatStyle: .balance)
    switch info.txType {
    case .erc721TransferFrom, .erc721SafeTransferFrom, .erc20Approve:
      return nil
    case .ethSend, .other:
      let amount = formatter.decimalString(for: info.ethTxValue.removingHexPrefix, radix: .hex, decimals: Int(networkStore.selectedChain.decimals)) ?? ""
      let fiat = currencyFormatter.string(from: NSNumber(value: assetRatios[networkStore.selectedChain.symbol.lowercased(), default: 0] * (Double(amount) ?? 0))) ?? "$0.00"
      return fiat
    case .erc20Transfer:
      if let value = info.txArgs[safe: 1], let token = token(for: info.ethTxToAddress) {
        let amount = formatter.decimalString(for: value.removingHexPrefix, radix: .hex, decimals: Int(token.decimals)) ?? ""
        let fiat = currencyFormatter.string(from: NSNumber(value: assetRatios[token.symbol.lowercased(), default: 0] * (Double(amount) ?? 0))) ?? "$0.00"
        return fiat
      } else {
        return "$0.00"
      }
    @unknown default:
      return nil
    }
  }
  
  /// The gas fee including the fiat for the transaction, ex. `("0.003402 ETC", "$15.57")`
  private var gasFee: (String, fiat: String)? {
    let isEIP1559Transaction = info.isEIP1559Transaction
    let limit = info.ethTxGasLimit
    let formatter = WeiFormatter(decimalFormatStyle: .gasFee(limit: limit.removingHexPrefix, radix: .hex))
    let hexFee = isEIP1559Transaction ? (info.txDataUnion.ethTxData1559?.maxFeePerGas ?? "") : info.ethTxGasPrice
    if let value = formatter.decimalString(for: hexFee.removingHexPrefix, radix: .hex, decimals: Int(networkStore.selectedChain.decimals)) {
      return (value, {
        guard let doubleValue = Double(value), let assetRatio = assetRatios[networkStore.selectedChain.symbol.lowercased()] else {
          return "$0.00"
        }
        return currencyFormatter.string(from: NSNumber(value: doubleValue * assetRatio)) ?? "$0.00"
      }())
    }
    return nil
  }
  
  /// The formatted transaction fee for the transaction, ex. `"0.003401\n$15.57"`
  private var transactionFee: String? {
    guard let (fee, fiat) = gasFee else {
      return nil
    }
    let symbol = networkStore.selectedChain.symbol
    return String(format: "%@ %@\n%@", fee, symbol, fiat)
  }
  
  /// The market price for the asset
  private var marketPrice: String {
    let symbol = networkStore.selectedChain.symbol
    let marketPrice = currencyFormatter.string(from: NSNumber(value: assetRatios[symbol.lowercased(), default: 0])) ?? "$0.00"
    return marketPrice
  }

  private func namedAddress(for address: String) -> String {
    NamedAddresses.name(for: address, accounts: keyringStore.allAccounts)
  }

  private var title: String {
    switch info.txType {
    case .erc20Approve:
      return Strings.Wallet.transactionUnknownApprovalTitle
    default:
      return info.isSwap ? Strings.Wallet.swap : Strings.Wallet.sent
    }
  }

  private var header: some View {
    TransactionHeader(
      fromAccountAddress: info.fromAddress,
      fromAccountName: namedAddress(for: info.fromAddress),
      toAccountAddress: info.ethTxToAddress,
      toAccountName: namedAddress(for: info.ethTxToAddress),
      originInfo: info.originInfo,
      transactionType: title,
      value: value,
      fiat: fiat
    )
      .frame(maxWidth: .infinity)
      .padding(.vertical, 30)
  }
  
  var body: some View {
    NavigationView {
      List {
        Section(
          header: header
            .resetListHeaderStyle()
            .osAvailabilityModifiers { content in
              if #available(iOS 15.0, *) {
                content // Padding already applied
              } else {
                content.padding(.top)
              }
            }
        ) {
          if let transactionFee = transactionFee {
            detailRow(title: Strings.Wallet.transactionDetailsTxFeeTitle, value: transactionFee)
          }
          detailRow(title: Strings.Wallet.transactionDetailsMarketPriceTitle, value: marketPrice)
          detailRow(title: Strings.Wallet.transactionDetailsDateTitle, value: dateFormatter.string(from: info.createdTime))
          if !info.txHash.isEmpty {
            Button(action: {
              if let baseURL = self.networkStore.selectedChain.blockExplorerUrls.first.map(URL.init(string:)),
                 let url = baseURL?.appendingPathComponent("tx/\(info.txHash)") {
                openWalletURL?(url)
              }
            }) {
              detailRow(title: Strings.Wallet.transactionDetailsTxHashTitle) {
                HStack {
                  Text(info.txHash.truncatedHash)
                  Image(systemName: "arrow.up.forward.square")
                }
                  .foregroundColor(Color(.braveBlurpleTint))
              }
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
          detailRow(title: Strings.Wallet.transactionDetailsNetworkTitle, value: networkStore.selectedChain.chainName)
          detailRow(title: Strings.Wallet.transactionDetailsStatusTitle) {
            HStack(spacing: 4) {
              Image(systemName: "circle.fill")
                .foregroundColor(info.txStatus.color)
                .imageScale(.small)
                .accessibilityHidden(true)
              Text(info.txStatus.localizedDescription)
                .foregroundColor(Color(.braveLabel))
                .multilineTextAlignment(.trailing)
            }
            .accessibilityElement(children: .combine)
            .font(.caption.weight(.semibold))
          }
        }
        .listRowInsets(.zero)
      }
      .listStyle(.insetGrouped)
      .background(Color(.braveGroupedBackground).edgesIgnoringSafeArea(.all))
      .navigationTitle(Strings.Wallet.transactionDetailsTitle)
      .navigationBarTitleDisplayMode(.inline)
      .navigationViewStyle(.stack)
      .toolbar {
        ToolbarItemGroup(placement: .confirmationAction) {
          Button(action: { presentationMode.dismiss() }) {
            Text(Strings.done)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
  }
  
  private func detailRow(title: String, value: String) -> some View {
    detailRow(title: title) {
      Text(value)
        .multilineTextAlignment(.trailing)
    }
  }
  
  private func detailRow<ValueView: View>(title: String, @ViewBuilder valueView: () -> ValueView) -> some View {
    HStack {
      Text(title)
      Spacer()
      valueView()
    }
    .font(.caption)
    .foregroundColor(Color(.braveLabel))
    .padding(.horizontal)
    .padding(.vertical, 12)
    .listRowBackground(Color(.secondaryBraveGroupedBackground))
  }
}

#if DEBUG
struct TransactionDetailsView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      TransactionDetailsView(
        info: .previewConfirmedSend,
        networkStore: .previewStore,
        keyringStore: .previewStore,
        visibleTokens: [.previewToken],
        allTokens: [],
        assetRatios: ["eth": 4576.36],
        currencyCode: CurrencyCode.usd.code
      )
        .previewColorSchemes()
      TransactionDetailsView(
        info: .previewConfirmedSwap,
        networkStore: .previewStore,
        keyringStore: .previewStore,
        visibleTokens: [.previewToken],
        allTokens: [],
        assetRatios: ["eth": 4576.36],
        currencyCode: CurrencyCode.usd.code
      )
        .previewColorSchemes()
      TransactionDetailsView(
        info: .previewConfirmedERC20Approve,
        networkStore: .previewStore,
        keyringStore: .previewStore,
        visibleTokens: [.previewToken],
        allTokens: [],
        assetRatios: ["eth": 4576.36],
        currencyCode: CurrencyCode.usd.code
      )
        .previewColorSchemes()
    }
  }
}
#endif
