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
  
  @ObservedObject var transactionDetailsStore: TransactionDetailsStore
  @ObservedObject var networkStore: NetworkStore
  
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.openWalletURLAction) private var openWalletURL
  
  init(
    transactionDetailsStore: TransactionDetailsStore,
    networkStore: NetworkStore
  ) {
    self.transactionDetailsStore = transactionDetailsStore
    self.networkStore = networkStore
  }
  
  private let dateFormatter = DateFormatter().then {
    $0.dateFormat = "h:mm a - MMM d, yyyy"
  }

  private var header: some View {
    TransactionHeader(
      fromAccountAddress: transactionDetailsStore.parsedTransaction?.fromAddress ?? "",
      fromAccountName: transactionDetailsStore.parsedTransaction?.namedFromAddress ?? "",
      toAccountAddress: transactionDetailsStore.parsedTransaction?.toAddress ?? "",
      toAccountName: transactionDetailsStore.parsedTransaction?.namedToAddress ?? "",
      originInfo: transactionDetailsStore.parsedTransaction?.transaction.originInfo,
      transactionType: transactionDetailsStore.title ?? "",
      value: transactionDetailsStore.value ?? "",
      fiat: transactionDetailsStore.fiat
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
          if let transactionFee = transactionDetailsStore.gasFee {
            detailRow(title: Strings.Wallet.transactionDetailsTxFeeTitle, value: transactionFee)
          }
          if let marketPrice = transactionDetailsStore.marketPrice {
            detailRow(title: Strings.Wallet.transactionDetailsMarketPriceTitle, value: marketPrice)
          }
          detailRow(title: Strings.Wallet.transactionDetailsDateTitle, value: dateFormatter.string(from: transactionDetailsStore.transaction.createdTime))
          if !transactionDetailsStore.transaction.txHash.isEmpty {
            Button(action: {
              if let baseURL = self.networkStore.selectedChain.blockExplorerUrls.first.map(URL.init(string:)),
                 let url = baseURL?.appendingPathComponent("tx/\(transactionDetailsStore.transaction.txHash)") {
                openWalletURL?(url)
              }
            }) {
              detailRow(title: Strings.Wallet.transactionDetailsTxHashTitle) {
                HStack {
                  Text(transactionDetailsStore.transaction.txHash.truncatedHash)
                  Image(systemName: "arrow.up.forward.square")
                }
                  .foregroundColor(Color(.braveBlurpleTint))
              }
            }
            .listRowBackground(Color(.secondaryBraveGroupedBackground))
          }
          if let network = transactionDetailsStore.network {
            detailRow(title: Strings.Wallet.transactionDetailsNetworkTitle, value: network.chainName)
          }
          detailRow(title: Strings.Wallet.transactionDetailsStatusTitle) {
            HStack(spacing: 4) {
              Image(systemName: "circle.fill")
                .foregroundColor(transactionDetailsStore.transaction.txStatus.color)
                .imageScale(.small)
                .accessibilityHidden(true)
              Text(transactionDetailsStore.transaction.txStatus.localizedDescription)
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
      .onAppear(perform: transactionDetailsStore.update)
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
